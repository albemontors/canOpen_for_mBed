Jose and Alberto library for canOpen CIA301 nearly spec

This is a work in progress but it is in running conditions as it is over here

================================================
                 INIT TUTORIAL
================================================

To get started add this init at the beginning of your main.c:

    #include "JAcan.h"
    
Now go to "JAcan.h" and edit the PINOUT RD and TD for your own board.
Also modify the FREQUENCY if needed.
NODE_NUMBER referes to the amount of nodes that have PDOs exchange,
it will be addressed later.

Declare the following function somewhere in your "main.cpp", it can be below your main:

    void can_init(){

        PDO_Dictionary_Entry PDO_Dictionary_init[NODE_NUMBER];

        // ======= USER INIT SECTION ======== //

        bool isMaster_init = 1; // set to 0 on slave devices

        // PDO SETUP EXAMPLE 
    /* (remove this line to use the PDOs)
        //-------------- First node defined, ID does not have to be 0
        PDO_Dictionary_init[0].id = 0x10; // id of the can node
        PDO_Dictionary_init[0].pdo[0].code = XPDOX; // id of the PDO (replace the Xs to match the desired PDO, example: TPDO2)
        PDO_Dictionary_init[0].pdo[0].data = &mydata; // address of the data to be read or written
        PDO_Dictionary_init[0].pdo[1].code = XPDOX; // id of the PDO (PDO definition does not have to be consecutive)
        PDO_Dictionary_init[0].pdo[1].data = &mydata; // address of the data to be read or written
        //--------------
        PDO_Dictionary_init[1].id = 0x10; // id of the can node
        PDO_Dictionary_init[1].pdo[0].code = TPDO1; // id of the PDO
        PDO_Dictionary_init[1].pdo[0].data = &mydata; // address of the data to be read or written
    */ (remove this line to use the PDOs)

        // ==== END OF USER INIT SECTION ===== // 
        
        can_setup(PDO_Dictionary_init);
        
    }
    
call "can_init()" at the beginning of your code and you can interface is now working.


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
    mydata.U64 = //any 64 bit int here
or:
    mydata.U32[0] //any 32 bit int here
    mydata.U32[1] //any other 32 bit int here
or:
    mydata.U16[0] //you get the idea

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
Make sure to match the NODE_NUMBER with the number of NODEs, that is "your highest node index" + 1.
Leave any unused PDO entry blank.

Now you are setup and the library will handle the PDOS that match that ID and CODE
and write or read them directly from the memory of your program.
