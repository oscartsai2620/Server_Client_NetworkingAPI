// your PA3 client code here
#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>

#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"

// ecgno to use for datamsgs
#define EGCNO 1

using namespace std;


void patient_thread_function (BoundedBuffer* request_buffer, int request_num, int patient_num) {
    // functionality of the patient threads
    //initialize a data point time that we want to obtain 
    double data_time = 0;

    //for each thread, we are requesting n numbers of data
    for(int i = 0; i < request_num; i++){
        //get a variable to store the data message, given the patient number/person, time, and since we are only obtaining the ecg1 values, we use the EGCNO defined as 1 to pass in as the ecg number
        datamsg data(patient_num, data_time, EGCNO);
        //push each requesting data into the request buffer
        request_buffer->push((char*) &data, sizeof(datamsg));
        data_time += 0.004; //increment the data time, since each time in the data increases by 0.04 at once 
    }

}

void file_thread_function (BoundedBuffer* request_buffer, TCPRequestChannel* chan, string filename, int msgCap) {
    // functionality of the file thread
	// sending a file msg to get length of file
	// create a filemsg, buf, and use cwrite to write the message
	filemsg fm(0, 0); //create file msag with offset and lenght = 0
	int length = sizeof(filemsg) + (filename.size() + 1); //get the length in binary of the filemsg + the filename + 1 (+1 because we are including /0)	
	char* buf = new char[length]; //create a buffer with the length to store data that we need
	memcpy(buf, &fm, sizeof(filemsg)); //first memory cpy the data of fm to the buffer pointer
	strcpy(buf + sizeof(filemsg), filename.c_str()); //buffer pointer + filemsg will point to the dest that need to add the filename + \0, which strcpy copys the filename string pointer with \0 to that dest.
	//c_str() returns the pointer of a sequence of array(string) with a \0 (null terminated sequence)
	chan->cwrite(buf, length); //client writes the data of fm(0,0) + filename + \0 to the server which will return the length of the file we want
	
	// receiving response, i.e., the length of the file
	// Hint: use 
	__int64_t filesize = 0; //initalize an __int64_t variable to store the size of file
	chan->cread(&filesize, sizeof(__int64_t)); //retrieve size of file

	string recieve_file = "received/" + filename;
    //allocate the file in memory
    FILE* openedFILE = fopen(recieve_file.c_str(), "w");
    //seek operation to touch all bytes tha need tot be allocated
    fseek(openedFILE, filesize, SEEK_SET);
    //close file
    fclose(openedFILE);

    filemsg* msg = (filemsg*) buf;
    long int dataleft = filesize;

    //dealing with requests
    while((int)dataleft > 0){
        //cout << "PUSH DATA INTO REQUEST" << endl;
        //create filemsg
        if((int)dataleft < msgCap){ //if its the last bit of data that doesn't complete a full message cap
            msg->length = (int)dataleft;
        }
        else{ //if can still get a max length of message
            msg->length = msgCap;
        }
        //push file message to request buffer with the file name
        request_buffer->push((char*) msg, length); 

        //increment the offset and update left over data length
        msg->offset += msg->length;
        dataleft -= msg->length;
    }
    //deallocate
    delete[] buf;
}

void worker_thread_function (BoundedBuffer* request_buffer, BoundedBuffer* response_buffer, TCPRequestChannel* workChan, int buff_size) {
    // functionality of the worker threads
    //declare a pointer to store the popped request item
    char request[1024];
    //declare a double to store the data obtain from server
    double obtain_ecg;

    //pop the request from request buffer
    request_buffer->pop(request, 1024);
    //change to a message type to check what kind of message we are requesting (file or data)
    MESSAGE_TYPE* msg = (MESSAGE_TYPE*) request;

    //loop until we obtain QUIT_MSG indicating all requests have been processed
    while(*msg != QUIT_MSG){
        //if it is a data message
        if(*msg == DATA_MSG){
            //send request to server
            workChan->cwrite(request, sizeof(datamsg));
            //obtain the data back from server
            workChan->cread(&obtain_ecg, sizeof(double));

            //create a pair structure of the data obtain with the patient number
            datamsg* message = (datamsg*) request;
            pair<int, double> data = {message->person, obtain_ecg};

            //push data into response buffer
            response_buffer->push((char*) &data, sizeof(data));
        }
        //if it is a file message
        else if(*msg == FILE_MSG){
            //create a buffer to get the file from server
            char* fileBuff = new char[buff_size];

            //get the name of the file
            filemsg* f_msg = (filemsg*) request;
            //offset the pointer 1 because when we push the filemsg to the request_buffer we also put the file name after the message
            string filename = (char*) (f_msg + 1);

            //get the length of the requests
            unsigned long int f_length = sizeof(filemsg) + (filename.size() + 1);
            //ask server for the request data
            workChan->cwrite(request, f_length);
            //recieves portions of the file (some data)
            workChan->cread(fileBuff, f_msg->length);

            //open/create file in update mode (r+) means Truncate with initial position at the beginning
            string recieve_file = "received/" + filename;
            FILE* openedFILE = fopen(recieve_file.c_str(), "r+");

            //using fseek to update the file with correct offset
            //fseek with SEEK_SET opens the file on a set position giving the offset value, here offset is where we want to insert our data to the correct position
            fseek(openedFILE, f_msg->offset, SEEK_SET);
            //write the data in fileBuff, 1 byte at a time for f_msg->length amount of times, from the pointer at openedFILE (which is the pointer points to the file at the offset)
            //cout << "WRITE" << endl;
            fwrite(fileBuff, 1, f_msg->length, openedFILE);
            //close the file
            fclose(openedFILE);

            //deallocate memory
            delete[] fileBuff;

        }
        //if it is unknown message
        else{
            cout << "Unknown request" << endl;
            exit(1);
        }

        //get the next request from buffer
        request_buffer->pop(request, 1024);
        msg = (MESSAGE_TYPE*) request;
    }
    //Send quit message to server
    workChan->cwrite(msg, sizeof(QUIT_MSG));

}

void histogram_thread_function (BoundedBuffer* response_buffer, HistogramCollection * collection) {
    // functionality of the histogram threads
    pair<int, double> data;

    //pop the response from the buffer
    response_buffer->pop((char*) &data, sizeof(data));

    //while the data person is greater than 0 (a valid person), cuz we set the first value as the patient number
    while(data.first > 0){
        //update the histogram
        collection->update(data.first, data.second);

        //pop the next response
        response_buffer->pop((char*) &data, sizeof(data));
    }
}


int main (int argc, char* argv[]) {
    int n = 1000;	// default number of requests per "patient"
    int p = 10;		// number of patients [1,15]
    int w = 100;	// default number of worker threads
	int h = 20;		// default number of histogram threads
    int b = 20;		// default capacity of the request buffer (should be changed)
	int m = MAX_MESSAGE;	// default capacity of the message buffer
	string f = "";	// name of file to be transferred
    string host = "127.0.0.1"; //IP address
    string portnum = "10000"; // port number
    
    // read arguments
    int opt;
	while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:a:r:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
                break;
			case 'p':
				p = atoi(optarg);
                break;
			case 'w':
				w = atoi(optarg);
                break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
                break;
			case 'm':
				m = atoi(optarg);
                break;
			case 'f':
				f = optarg;
                break;
            //add a case for -a flag
            case 'a':
                host = optarg;
                break;
            //add a case for -r flag
            case 'r':
                portnum = optarg;
                break;

		}
	}
    
	// fork and exec the server
    // int pid = fork();
    // if (pid == 0) {
    //     execl("./server", "./server", "-m", (char*) to_string(m).c_str(), nullptr);
    // }
    
	// initialize overhead (including the control channel)
	TCPRequestChannel* chan = new TCPRequestChannel(host, portnum);
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;

    // making histograms and adding to collection
    for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }
	
	// record start time
    struct timeval start, end;
    gettimeofday(&start, 0);

    /* create all threads here */
    //make a vector of worker channels
    vector<TCPRequestChannel*> worker_channels;
    for(int i = 0; i < w; i++){
        //create a new channel and push the new channel to our worker channels
        TCPRequestChannel* newChan = new TCPRequestChannel(host, portnum);
        worker_channels.push_back(newChan);
    }
    //create threads for patients, histograms, and workers
    thread* pat_threads = new thread[p];
    thread* work_thread = new thread[w];
    thread* hist_thread = new thread[h];
    thread file_thread;

    if(f.empty()){ //if not requesting file, means requesting data points
        //patients threads
        for(int i = 0; i < p; i++){
            pat_threads[i] = thread(patient_thread_function, &request_buffer, n, i+1);
        }
        //worker threads
        for(int i = 0; i < w; i++){
            work_thread[i] = thread(worker_thread_function, &request_buffer, &response_buffer, worker_channels[i], m);
        }
        //histogram threads
        for(int i = 0; i < h; i++){
            hist_thread[i] = thread(histogram_thread_function,&response_buffer, &hc);
        }
    }
    else{ //else if requesting for file
        //start file thread
        file_thread = thread(file_thread_function, &request_buffer, chan, f, m);
        //start worker threads
        for(int i = 0; i < w; i++){
            work_thread[i] = thread(worker_thread_function, &request_buffer, &response_buffer, worker_channels[i], m);
        }
    }

	/* join all threads here */
    if(f.empty()){ //if not requesting file, means requesting data points
        //patients threads
        for(int i = 0; i < p; i++){
            pat_threads[i].join();
        }
        //push quit msgs for each worker threads to request buffer for indicating no more requests
        for(int i = 0; i < w; i++){
            MESSAGE_TYPE quit = QUIT_MSG;
            request_buffer.push((char*) &quit, sizeof(quit));
        }
        //worker threads
        for(int i = 0; i < w; i++){
            work_thread[i].join();
        }
        //set up data points act as QUIT_MSGs for the histogram threads
        pair<int, double> quit_data = {-1, -1};
        for(int i = 0; i < h; i++){
            response_buffer.push((char*) &quit_data, sizeof(quit_data));
        }
        //histogram threads
        for(int i = 0; i < h; i++){
            hist_thread[i].join();
        }
    }
    else{//join if file threads are working
        //join file thread
        file_thread.join();
        //push quit msgs for each worker threads to request buffer for indicating no more requests
        for(int i = 0; i < w; i++){
            MESSAGE_TYPE quit = QUIT_MSG;
            request_buffer.push((char*) &quit, sizeof(quit));
        }
        //join worker threads
        for(int i = 0; i < w; i++){
            work_thread[i].join();
        }
    }
	// record end time
    gettimeofday(&end, 0);

    // print the results
	if (f == "") {
		hc.print();
	}
    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

	// quit and close control channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!" << endl;
    delete chan;

	// wait for server to exit
	//wait(nullptr);
    delete[] pat_threads;
    delete[] work_thread;
    delete[] hist_thread;

    for(int i = 0; i < w; i++){
        delete worker_channels[i];
    }
}
