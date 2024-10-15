/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Please include your Name, UIN, and the date below
	Name: Tom Shwartz
	UIN: 232009840
	Date: 9/19/2024
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <string> 
#include <sstream>
#include <chrono>

using namespace std;
using namespace std::chrono;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	string filename = "";

	//Add other arguments here
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}

	 pid_t cpid = fork();
     if (cpid == -1){
        return -1;
	 }
	
	if (cpid == 0) {
		const char* path = "./server";
        char* args[] = {const_cast<char*>(path), nullptr}; 
        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
    

	//Task 1:
	//Run the server process as a child of the client process

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	//Task 4:
	//Request a new channel
	MESSAGE_TYPE new_channel_msg = NEWCHANNEL_MSG;
	chan.cwrite(&new_channel_msg, sizeof(MESSAGE_TYPE));

	char new_channel_name[100];
	chan.cread(new_channel_name, sizeof(new_channel_name)); 

	FIFORequestChannel new_chan(new_channel_name, FIFORequestChannel::CLIENT_SIDE);
	
	//Task 2:
	//Request data points
	if (!filename.empty()) {
		system("mkdir -p received");

		filemsg fm(0, 0);
		string fname = filename;

		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);
		delete[] buf2;

		__int64_t file_length;
		chan.cread(&file_length, sizeof(__int64_t));
		cout << "The length of " << fname << " is " << file_length << endl;

		auto start = high_resolution_clock::now();

		char buffer[MAX_MESSAGE];
		__int64_t offset = 0;
		ofstream outFile("received/" + fname, ios::binary);

		while (offset < file_length) {
			int chunk_size = min(MAX_MESSAGE, static_cast<int>(file_length - offset));

			filemsg fm_chunk(offset, chunk_size);
			memcpy(buffer, &fm_chunk, sizeof(filemsg));
			strcpy(buffer + sizeof(filemsg), fname.c_str());

			chan.cwrite(buffer, sizeof(filemsg) + fname.size() + 1);
			chan.cread(buffer, chunk_size);

			outFile.write(buffer, chunk_size);
			offset += chunk_size;
		}

		outFile.close();

		auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        cout << "File transfer for " << fname << " completed in " << duration << " ms." << endl;
	}
	else {
		if (p != -1 && t != -1 && e != -1) {
            char buf[MAX_MESSAGE];
			datamsg x(p, t, e);
			
			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			double reply;
			chan.cread(&reply, sizeof(double));
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		}
		else if (p != -1) {
			system("mkdir -p received");

			ofstream received("received/x1.csv");
			if (!received.is_open()) {
				cerr << "Failed to open x1.csv for writing" << endl;
				return -1;  
			}

			string path = "BIMDC/9.csv";  
			ifstream bimdc(path);
			if (!bimdc.is_open()) {
				cerr << "Failed to open " << path << endl;
				return -1;  
			}

			string line;
			int count = 0;

			while (count < 1000 && getline(bimdc, line)) {
				stringstream ss(line);
				string time_str;
				getline(ss, time_str, ',');

				t = stof(time_str);

				for (int ecg = 1; ecg <= 2; ++ecg) {
					datamsg x(p, t, ecg);
					chan.cwrite(&x, sizeof(datamsg));  
					double reply;
					chan.cread(&reply, sizeof(double));  

					if (ecg == 1) {
						received << t << ", " << reply;  
					} else {
						received << ", " << reply << endl;  
					}
				}

				count++;  
			}

			
			bimdc.close();
			received.close();
		}

	}

    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	new_chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}
