/*
This cpp file is used for testing the functionality of the interaction between client with motor & rfid reader,  and server with traffic signal .

Usage:
1. install g++
2. Create static library by: *MAKE SURE that CMFRC522 is in the same directory with RFID_test*
    g++ -c CMFRC522.cpp
    ar rcs CMFRC522.a CMFRC522.o
3.  Complie:
    g++ motorclient.cpp -o motorclient -lpigpio -lrt -lpthread -L. -l:CMFRC522.a
4. Run:
    sudo ./motorclient

*/
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "CMFRC522.h"
#include "pigpio.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <stdio.h>

using namespace std;


//Global constant for I2C address on PCA9685 
const int PCA9685_ADDR = 0x40;				//define the PCA9685 communication and activate i2c through pigpio
class PCA9685 {
private:
    int i2cHandle;

public:
    PCA9685(int handle) : i2cHandle(handle) {}

    void setup() {					//setup and initialize the i2c communication
        i2cWriteByteData(i2cHandle, 0x00, 0x20);
        int freq = 1000;
        int prescale = (25000000 / (4096 * freq)) - 1;
        i2cWriteByteData(i2cHandle, 0xFE, prescale); // Set PWM frequency to 1000 Hz
    }

    void setPWM(int channel, int on, int off) {		//set a function to write information via i2C
        int regBase = 0x06 + 4 * channel;
        i2cWriteByteData(i2cHandle, regBase, on & 0xFF);
        i2cWriteByteData(i2cHandle, regBase + 1, on >> 8);
        i2cWriteByteData(i2cHandle, regBase + 2, off & 0xFF);
        i2cWriteByteData(i2cHandle, regBase + 3, off >> 8);
    }
};
class Motor {
private:
    PCA9685& pca;

public:
    Motor(PCA9685& pcaController) : pca(pcaController) {}

    void Forward() {			// set the parameters for different motion control
        pca.setPWM(0, 0, 0);
        pca.setPWM(1, 1024, 0);
        pca.setPWM(3, 0, 0);
        pca.setPWM(2, 1024, 0);
        pca.setPWM(4, 0, 0);
        pca.setPWM(5, 1024, 0);
        pca.setPWM(6, 0, 0);
        pca.setPWM(7, 1024, 0);
    }

    void Backward() {
        pca.setPWM(0, 1024, 0);
        pca.setPWM(1, 0, 0);
        pca.setPWM(3, 1024, 0);
        pca.setPWM(2, 0, 0);
        pca.setPWM(4, 1024, 0);
        pca.setPWM(5, 0, 0);
        pca.setPWM(6, 1024, 0);
        pca.setPWM(7, 0, 0);
    }

    void TurnLeft() {
        pca.setPWM(0, 0, 0);
        pca.setPWM(1, 0, 0);
        pca.setPWM(3, 0, 0);
        pca.setPWM(2, 0, 0);
        pca.setPWM(4, 0, 0);
        pca.setPWM(5, 512, 0);
        pca.setPWM(6, 0, 0);
        pca.setPWM(7, 512, 0);
    }

    void TurnRight() {
        pca.setPWM(0, 0, 0);
        pca.setPWM(1, 512, 0);
        pca.setPWM(3, 0, 0);
        pca.setPWM(2, 512, 0);
        pca.setPWM(4, 0, 0);
        pca.setPWM(5, 0, 0);
        pca.setPWM(6, 0, 0);
        pca.setPWM(7, 0, 0);
    }

    void Stop() {
        pca.setPWM(0, 0, 0);
        pca.setPWM(1, 0, 0);
        pca.setPWM(3, 0, 0);
        pca.setPWM(2, 0, 0);
        pca.setPWM(4, 0, 0);
        pca.setPWM(5, 0, 0);
        pca.setPWM(6, 0, 0);
        pca.setPWM(7, 0, 0);
    }
};

class CppThread { 		//thread wrapper for future use 

public:
	inline void start() {
		uthread = std::thread(&CppThread::run, this);
	}

	inline void join() {
		uthread.join();
	}

protected:
	virtual void run() = 0;	

private:
	// pointer to the thread
	std::thread uthread;
};


class Client{			
    private:
        
        char* rdmsg =new char {5};
        bool sig=1;
        const char entermsg[9]="Entered";
        const char exitmsg[7]="Exited";
    
    
    public:

    int sock;
    int flag;
    int bytesRead;
    int byte_rd(){ return read(sock,rdmsg,5);};   //Read the byte number of the message in socket
 

    void setupSocket(const std::string& host, int port) {  //setup socket connection with server by host address and port number
    sock = socket(AF_INET, SOCK_STREAM, 0);
    std::cout<<"I am here in setupsocket"<<std::endl;
    if (sock < 0) {
        std::cout << "Socket creation error "<<std::endl;
         
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address/ Address not supported "<<std::endl;
        
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection Failed "<<std::endl;
    
    }else if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) > 0)
    {
        std::cout<<"Connected"<<std::endl;
    }
    
    }

    void sendmsg() { 						//function to send a message depending on whether the car has entered or exited the zone
        if (flag==0)
        {
            send(sock, entermsg,strlen(entermsg), 0);
        }else{
            send(sock, exitmsg,strlen(exitmsg), 0);
        }
        //std::cout<<"Notified Server!!"<<std::endl;

    }
     bool sigiden() {							//function to identify signal sent by the server and recreate one for client
        if(strcmp(rdmsg,"RED  ")==0)
        {
            sig=0;
            std::cout<<"The current boolean signal is:"<<sig<<std::endl;
        }
        else if (strcmp(rdmsg,"GREEN")==0)
        {
           sig=1;
            std::cout<<"The current boolean signal is:"<<sig<<std::endl;
        }
        
            
    return sig;           
    }

    void readsig(){							 //Function to read data and handle error
        
        std::cout<<"Reading data..."<<std::endl;
        memset(rdmsg, 0, 5);
        bytesRead = read(sock,rdmsg,5);
        read(sock,rdmsg,5);
        if (bytesRead < 0) {
        // Error handling
            std::cerr << "Error reading from socket" << std::endl;
            delete[] rdmsg; // Free allocated memory
            return ;
        } else if (bytesRead == 0) {
        // Connection closed
            std::cerr << "Connection closed by peer" << std::endl;
            delete[] rdmsg; // Free allocated memory
            return ;
        } else {
        // Read successful
            std::cout<<rdmsg<<std::endl;
            sigiden();
        }
    
    
 
    

}
};
//RFIDDetection Thread: to detect whether car is present and store the message into a character array get_id[16]
class RFIDThread : public CppThread {

private:
	// implement run which is doing all the work
    Client& clie;
	unsigned char get_id[16];
    int enter_id = 68 ;
    int exit_id = 189 ;

    

public:
    void run()  {
         CMFRC522 mfrc522;
         mfrc522.PCD_Init();
	     while(1) { 
        std::cout << "Detecting..." << std::endl;
        if (mfrc522.isNewCardPresent()==0) { // Detect whether a new card is present
            std::cout << "Card Detected/n The card id is:" << std::endl;
            mfrc522.RC522_Anticoll(mfrc522._id);
            for (uint8_t i = 0; i < 8; i++) {
                get_id[i]= mfrc522._id[i];
                std::cout<<std::hex<<int(mfrc522._id[i])<<" "; //print out the result
            }
          std::cout<<endl;
        }

        if(get_id[2]==enter_id)				// store boolean value flag in class client by 0 or 1 depending on the get_id from rfid card(exter-enter_id or exit-exit_id 
        {
            clie.flag=0;
            std::cout<<"Entered Crossing!!"<<std::endl;
        }else if(get_id[2]==exit_id)
        {
            clie.flag=1;
            std::cout<<"Exited Crossing!!"<<std::endl;

        }
        clie.sendmsg();
        std::this_thread::sleep_for(500ms);
        }
	}

RFIDThread(Client& client) : clie(client){}
    

};


class Readingmsg: public CppThread { //Thread Class to read the data depending on the number of byte read, that is not equal to 0
    private:
    Client& clie;

    public:
    void run()  {
        while(1)
        {  
            if(clie.byte_rd()!=0)
            {
            std::cout<<"already Read!!"<<std::endl;
            clie.readsig();
            }
            std::this_thread::sleep_for(500ms);
        }
    }
    
    Readingmsg(Client& Clie):clie(Clie){};
};

class Motioncntrl: public CppThread {	// Thread to implement the start and stop the motion of the motor depending on the received signal by server (referred as clie.sigden())
    
    private:
    Client& clie;
    Motor& mot;

    public:

    void run() {
        while(1){
            mot.Forward();
            if(clie.sigiden())
            {
             std::cout<<"Car Stopped!!!"<<std::endl;
             mot.Stop();
            }
        }

    }

    Motioncntrl(Client& Clie,Motor& Mot):clie(Clie),mot(Mot){};
};

int main(){
    const char host []= "192.168.1.132";//change the ip address of the server if required
    int port = 5560;// make sure the same port is used in server
    Client client;
    client.setupSocket(host, port);// create class 
    if(gpioInitialise()<0){
        std::cout<<"Pigpio initialization failed"<<std::endl;
        return 1;
    }
    int i2cHandle = i2cOpen(1, PCA9685_ADDR, 0);
    if (i2cHandle < 0) {
        std::cerr << "Failed to open I2C communication." << std::endl;
        return 1;
        
    }
    gpioSetISRFunc(18, RISING_EDGE, 0, gpioInterruptHandler);
    PCA9685 pca(i2cHandle);
    pca.setup();
    Motor motor(pca);
    RFIDThread rfidrd(client);
    Readingmsg rdingmsg(client);
    Motioncntrl motctl(client, motor);
    if (client.sock < 0) return -1;
    
    rfidrd.start(); //start and stop threads
    rdingmsg.start();
    motctl.start();
    rfidrd.join();
    rdingmsg.join();
    motctl.join();
    gpioTerminate();

    return 0;

}
