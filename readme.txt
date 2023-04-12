Jose and Alberto library for canOpen CIA301 nearly spec

This is a work in progress but it is in running conditions as it is over here
Additions are welcome, you are free to use this code for your own purposes.

================================================
            WHAT THIS LIBRARY DOES
================================================

This library provides a simple and effective method to send CAN frames.
This library also implements handling for PDOs.
A PDO is a packet that ships predefined data on request.

In the average CAN network a single master controls multiple slave devices.
Each slave device has 4 PDOs, each having a Transmit (TPDO) and Receive (RPDO) frame.
To receive data through a PDO you will need to allocate some memory for that data and pass
a pointer to that data to the init funcion, this will automatically update that data every
time the specific PDO is received from the bus.

This allows the user to interact with variables that are automatically updated by this library
without having to worry about updating that data in real time.

The same happens when sending a PDO, when the user sends the PDO they do not need to provide
the data to be sent. The relevant data is estrapolated directly from the memory of the device.

This is possible thanks to the PDO Dictionary.


================================================
                 INIT TUTORIAL
================================================

To get started add this init at the beginning of your main.c:

    #include "JAcan.h"
    
Now go to "JAcan.h" and edit the PINOUT RD and TD for your own board.
Also modify the FREQUENCY if needed.
NODE_NUMBER referes to the amount of nodes that have PDOs exchange,
it will be addressed later.

Declare the following function somewhere in your "main.cpp", it can be below your main,
instructions for editing the PDO Dictionary are provided below.

    void can_init(){

        PDO_Dictionary_Entry PDO_Dictionary_init[NODE_NUMBER];

        // ======= USER INIT SECTION ======== //

        bool isMaster_init = 1; // set to 0 on slave devices

        // PDO SETUP EXAMPLE 
    /* (remove this line to use the PDOs)
        //-------------- First node defined, ID does not have to match array index
        PDO_Dictionary_init[0].id = 0x10; // id of the can node
        PDO_Dictionary_init[0].pdo[0].code = XPDOX; // id of the PDO (replace the Xs to match the desired PDO, example: TPDO2)
        PDO_Dictionary_init[0].pdo[0].data = &mydata; // address of the data to be read or written
        PDO_Dictionary_init[0].pdo[1].code = XPDOX; // id of the PDO (PDO definition does not have to be consecutive)
        PDO_Dictionary_init[0].pdo[1].data = &mydata; // address of the data to be read or written
        //--------------
        PDO_Dictionary_init[1].id = 0x11; // id of the second can node we want to address
        PDO_Dictionary_init[1].pdo[0].code = TPDO1; // id of the PDO
        PDO_Dictionary_init[1].pdo[0].data = &mydata; // address of the data to be read or written
    */ (remove this line to use the PDOs)

        // ==== END OF USER INIT SECTION ===== // 
        
        can_setup(PDO_Dictionary_init, isMaster_init);
        
    }
    
call "can_init()" at the beginning of your code and your can interface is now working.


================================================
        TUTORIAL TO SEND OUT A PACKET
================================================

To send a can packet allocate a packet using:

    CANMessage* send = can_allocate();

then use the following to init the packet:

    CAN_Id id;
    id.bd.id = 0x10;    //put the correct node id
    id.bd.code = TPDO1; //put the correct PDO id
    send->id = id.raw;
    send->type = CANData;           //set this to "CANRemote" to send an RTR packet
    send->format = CANStandard;     //this sets the 11 bit address mode

define a PDO_Data, this type can be accessed as 1 u64, 2 U32, 4 U16 or 8 U8:

    PDO_Data mydata;
    mydata.U64 = //any 64 bit type here
or:
    mydata.U32[0] //any 32 bit type here
    mydata.U32[1] //any other 32 bit type here
or:
    mydata.U16[0] //any 16 bit type here
    ...
    mydata.U16[3] //any 16 bit type here
or:
    mydata.U8[0] //any 8 bit type here
    ...
    mydata.U8[7] //any 8 bit type here

A combination of types can be used as long as they dont overwrite sections of the memory
Example:
    mydata.U32[0] = 32bitsdata;
    mydata.U16[2] = 16bitsdata;
    mydata.U8[7] = 8bitsdata;

Data in this packet looks like this:
4 bytes contain 32bitsdata
2 bytes contain 16bitsdata
1 byte is embity
1 byte contains 8bitdata
TOTAL: 8 bytes which is the maximum
NOTE: editing mydata.U16[1] now will overwrite part of 32bitsdata

then write the data in the packet using:

    for(int i = 0; i < 8; i++) send->data[i] = mydata.U8[i];

finally send out the message using:

    can_send(send);



================================================
            TUTORIAL TO RECEIVE PDOS
================================================

Use the function can_init you setup in main to create the dictionary.
Set the value of "isMaster_init" to one for a Master, and to 0 for a Slave.
You have to set the value NODE_NUMBER in JAcan.h to the amount of nodes you have in the network,
this is explained better later.
For each pdo setup the ID of the sender node, the CODE(TPDO1 for example) of the pdo and
the pointer to the memory the pdo is destined for.

Remember that master devices send TPDOs and receive RPDOs while
              slave  devices send RPDOs and receive TPDOs

The PDO CODE does not have to match the position in the dictionary array,
fill the array starting from element 0 for each NODE and each PDO of the node from element 0 up to 7 (can be less than 7).
Make sure to set the NODE_NUMBER parameter to the number of NODEs, that is "your highest node index" + 1.
Leave any unused PDO entry blank.

Now you are setup and the library will handle the PDOS that match that ID and CODE
and write or read them directly from the memory of your program.


================================================
            TUTORIAL TO SEND PDOS
================================================

Follow the tutorial for receiveing PDO and configure the PDO that needs to be sent.

Define a CAN_Id and set the NodeId and CODE, these NEED to match one entries of the pdo dictionary
If the id and code requested do not match any dictionary entry, no pdo is sent.

    CAN_Id mypdo;
    mypdo.db.id = 0x10; // set the desired id
    mypdo.bd.code = TPDO1;  // change to the need

now you can use the sender funtion and the interface will read the corresponding
data in the memory and send out the packet.

    pdoSender(mypdo);

Remember that only TPDOs can be transmitted from a master and RPDOs from a slave.
This library will prevent the transmission of wrong packets.

================================================
        TUTORIAL TO SEND PDOS RTR REQUESTS
================================================

A PDO RTR request is a specific packet that requests a PDO to be sent from another node.
NOTE that the data sent by the interacting node is the one stated in their PDO Dictionary at that specific entry

When requesting a PDO it is not necessary to specify whether the PDO is Transmit (TPDO) or receive (RPDO)

If we want to receive RPDO2 from node 0x20 we need to send TPDO2 with RTR
Anyway, is only necessary to specify the number of the PDO (using PDO2), the library will sort the type for us
To do so:

    CAN_Id mypdorequest;
    mypdorequest.db.id = 0x20; // set the desired id
    mypdorequest.bd.code = PDO2;  // change to the need

now we can use the request function to send the request packet

    pdoRequest(mypdorequest);

Following, a packet with the requested PDO will be received from the bus.
It is mandatory to have a dictionary entry for the PDO that is received, for the data to be saved,
but it is not required to have a Dictionary entry for the pdo that is transmitted as a request.
For example we might request PDO3 and not have a Dictionary entry for TPDO3, but we need an entry
for RPDO3, or the received data will be lost upon reception.


================================================
                GENERALITIES
================================================

The JAcan.cpp contains the two variables:

bool verbose = 0;
bool verbose_low_level = 0;

Setting the values to 1 provide usefull informations through printf
This is usefull when setting up the dictionary because it verboses every received packets
even when they are not listed in the Dictionary helping the debug process of dictionary entries.
The usage of low level verbose should not be necessary when not modifing the source code.

Verbose headers:
(S) System message
(E) Error message
(!) Critical error message

Please refere to the definitions in JAcan.h for PDO codes when debugging.