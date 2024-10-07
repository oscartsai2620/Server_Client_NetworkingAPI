#include <thread>
#include "TCPRequestChannel.h"

using namespace std;


int buffercapacity = MAX_MESSAGE;
char* buffer = NULL; // buffer used by the server, allocated in the main

int nchannels = 0;
vector<string> all_data[NUM_PERSONS];


void populate_file_data (int person) {
	//cout << "populating for person " << person << endl;
	string filename = "BIMDC/" + to_string(person) + ".csv";
	char line[100];
	ifstream ifs(filename.c_str());
	if (ifs.fail()){
		EXITONERROR("Data file: " + filename + " does not exist in the BIMDC/ directory");
	}
	
	while (!ifs.eof()) {
		line[0] = 0;
		ifs.getline(line, 100);
		if (ifs.eof()) {
			break;
		}
		
		if (line[0]) {
			all_data[person-1].push_back(string(line));
		}
	}
}

double get_data_from_memory (int person, double seconds, int ecgno) {
	int index = (int) round(seconds / 0.004);
	string line = all_data[person-1][index]; 
	vector<string> parts = split(line, ',');
	
	double ecg1 = stod(parts[1]);
	double ecg2 = stod(parts[2]); 
	if (ecgno == 1) {
		return ecg1;
	}
	else {
		return ecg2;
	}
}

void process_file_request (TCPRequestChannel* rc, char* request) {
	filemsg f = *((filemsg*) request);
	string filename = request + sizeof(filemsg);
	filename = "BIMDC/" + filename; // adding the path prefix to the requested file name
	//cout << "Server received request for file " << filename << endl;

	if (f.offset == 0 && f.length == 0) { // means that the client is asking for file size
		__int64_t fs = get_file_size (filename);
		rc->cwrite ((char*) &fs, sizeof(__int64_t));
		return;
	}

	/* request buffer can be used for response buffer, because everything necessary have
	been copied over to filemsg f and filename*/
	char* response = request; 

	// make sure that client is not requesting too big a chunk
	if (f.length > buffercapacity) {
		cerr << "Client is requesting a chunk bigger than server's capacity" << endl;
		cerr << "Returning nothing (i.e., 0 bytes) in response" << endl;
		rc->cwrite(response, 0);
	}

	FILE* fp = fopen(filename.c_str(), "rb");
	if (!fp) {
		cerr << "Server received request for file: " << filename << " which cannot be opened" << endl;
		rc->cwrite(buffer, 0);
		return;
	}
	fseek(fp, f.offset, SEEK_SET);
	int nbytes = fread(response, 1, f.length, fp);

	/* making sure that the client is asking for the right # of bytes,
	this is especially imp for the last chunk of a file when the 
	remaining lenght is < buffercap of the client*/
	assert(nbytes == f.length); 

	rc->cwrite(response, nbytes);
	fclose(fp);
}

void process_data_request (TCPRequestChannel* rc, char* request) {
	datamsg* d = (datamsg*) request;
	double data = get_data_from_memory(d->person, d->seconds, d->ecgno);
	rc->cwrite(&data, sizeof(double));
}

void process_unknown_request (TCPRequestChannel* rc) {
	char a = 0;
	rc->cwrite(&a, sizeof(char));
}


void process_request (TCPRequestChannel* rc, char* _request) {
	MESSAGE_TYPE m = *((MESSAGE_TYPE*) _request);
	if (m == DATA_MSG) {
		usleep(rand() % 5000);
		process_data_request(rc, _request);
	}
	else if (m == FILE_MSG) {
		process_file_request(rc, _request);
	}
	else {
		process_unknown_request(rc);
	}
}

void handle_process_loop (TCPRequestChannel* channel) {
	/* creating a buffer per client to process incoming requests
	and prepare a response */
	char* buffer = new char[buffercapacity];
	if (!buffer) {
		EXITONERROR ("Cannot allocate memory for server buffer");
	}

	while (true) {
		int nbytes = channel->cread(buffer, buffercapacity);
		if (nbytes < 0) {
			cerr << "Client-side terminated abnormally" << endl;
			break;
		}
		else if (nbytes == 0) {
			cout << "Server could not read anything... Terminating" << endl;
			break;
		}

		MESSAGE_TYPE m = *((MESSAGE_TYPE*) buffer);
		if (m == QUIT_MSG) {  // note that QUIT_MSG does not get a reply from the server
			cout << "Client-side is done and exited" << endl;
			break;
		}
		process_request(channel, buffer);
	}
	delete[] buffer;
	delete channel;
}

int main (int argc, char* argv[]) {
	buffercapacity = MAX_MESSAGE;
	string portnum = "10000";
	int opt;
	while ((opt = getopt(argc, argv, "r:m:")) != -1) {
		switch (opt) {
			case 'm':
				buffercapacity = atoi(optarg);
				break;
			case 'r':
				portnum = optarg;
				break;
		}
	}

	srand(time_t(NULL));
	for (int i = 0; i < NUM_PERSONS; i++) {
		populate_file_data(i+1);
	}
	
	// FIXME: open socket with address="" and r from CLI
	//		  then enter infinite loop calling TCPReqChan::accept_conn() and 2nd TCP constructor
	//		  dispatching handle_process_loop thread for new channel
	TCPRequestChannel* control_channel = new TCPRequestChannel("", portnum);
	while (true) {
		//create new socket and thread variable 
		int newSocket;
		thread newThread;

		//accept a connection socket from the client on the server
		newSocket = control_channel->accept_conn();
		//create a new channel with the new accepted socket, since the socket was already established, we just have to pass in the socket 
		TCPRequestChannel* newChan = new TCPRequestChannel(newSocket);
		//create a new thread to handle the process with the new channel
		//the new chan is set to have the fd of the clients, therefore handling the process loop and start processing the incoming requests from each clients
		newThread = thread(handle_process_loop, newChan);
		//since the main thread is not blocked, it can accept more connection, use detach here to make threads work independtly, so we can continue accept more clients
		newThread.detach();
		
	}

	cout << "Server terminated unexpectedly" << endl;
	delete control_channel;


}
