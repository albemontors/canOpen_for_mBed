Jose and Alberto library for canOpen CIA301 nearly spec

================================================
                 INIT TUTORIAL
================================================

To get started call the function at the beginning of your main.c:

    #include "JAcan.h"

Declare the following function above your main:

    void can_init(){

        PDO_Dictionary_Entry PDO_Dictionary_init[NODE_NUMBER];

        // ======= USER INIT SECTION ======== //

        bool isMaster_init = 1; // set to 0 on slave devices

        /* PDO SETUP EXAMPLE 
        PDO_Dictionary_init[0].id = 0x10; // id of the can node
        PDO_Dictionary_init[0].pdo[0].code = TPDO1; // id of the PDO
        PDO_Dictionary_init[0].pdo[0].data = &mydata; // address of the data to be read or written
        */

        // ==== END OF USER INIT SECTION ===== // 
        
        can_setup(PDO_Dictionary_init);
        
    }
    
Now call can_init() at the beginning of your code and you can interface is working.


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

As a master use the function can_init you setup in main to create the dictionary.
You have to set the value NODE_NUMBER in JAcan.h to the amount of nodes you have.
For each pdo setup the ID of the sender node, the CODE(id) of the pdo and
the pointer to the memory the pdo is destined for.

Now you are setup and the library will handle the PDOS that match that ID and CODE
and write them directly in the memory of your MCU.
