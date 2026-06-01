/****************************************************************************
 Copyright (c) 2002 - 2016 Qualcomm Technologies International, Ltd.

 FILE:           
 csr_bt_hci_convert_auto_readme.txt

 DESCRIPTION:   
 Describes the following
 1. Introduction
 2. Design approch
 3. Usage of the perl scripts
 4. Auto generated files
 5. Supporting files
 6. Commands to auto generate

 REVISION:      $Revision: #1 $

 ****************************************************************************/

FILE
        readme.txt

CONTAINS
        A textual description of the new hci_convert module 


Introduction
============

The HCI convert module either turn an command structure into a stream
of bytes to be sent from the host, or it can convert the stream of
bytes to the host into an HCI event structure.

The command structures are normally passed to the corestack, and the event
structures are sent to the DM.  These structures are passed
in the standard message queues.  The hcishim decides when to call
HCI Convert, but guarantees that it will only call 'hcicmd_read_pdu'
when there is a complete PDU in the buffer.



Interface
=========

The interface to this module is based on two main functions:

CsrBtConvertHciEvent
--------------------

This is the command that is called from hcishim when it thinks that
there is a complete PDU in the buffer.  The function will
read the data from the buffer, and construct a PDU 
The pointer to the PDU (which has been successfully
pmalloc'd) will be returned on success (as well as TRUE).

If the PDU seems to be corrupted in some way (the length is wrong, or
it is an array PDU and the count does not match, we do not recognise
the opcode, or it is a broken sef PDU), the module will  set the return
pointer to NULL, and return FALSE.  


CsrBtConvertHciCommand
----------------------

This is the mirror function to the one above.  It converts the command
passed to it to a stream of bytes that are sent to the host.  The only
way that it can fail is if the structure that was given to it is
corrupt in some way, or if there is a problem with the buffer
sub-system.

If the structure appears to be corrupt, then I have chosen to panic
the chip.  We should only be sent these structures from our own code,
and if they are broken in some way, then there is little that we can
do.


Semantics
=========

A number of the PDUs have structures that are not single blocks of memory (
this is because the PDU is of variable length).  The way that this is 
normally handled is by having an array of pointers to blocks of memory.  Each 
'block' can contain one or more 'entries'.  The number of entries, size of 
entry and entries per block are all implied by the opcode or event.  The only 
other variable sized block like the command complete event, which might contain 
a pointer to some arguments (which again can be of a variable size).

For all of the variable sized sections of data that are passed to or from 
hci_convert, following semantics are assumed:


- All of the pointers that are in the array of pointers to blocks must be 
initialised to either NULL, or a pmalloc'd block.  This means that code can 
just dumbly walk along the array of pointers, passing each to pfree.  All of 
the variable length PDUs contain a count field that indicates the total 
number of entries.  This count is used when reading data from the structure, 
or allocating memory for the structure, but recommended not to rely on it to 
find out how much memory has been allocated.  If there is less data than the 
count implies, handle this case gracefully (leave the buffer filled with crap
, and do not issue any warning / fault).


- Its assumed that the pointer to the argument block of the command complete 
event is either set to NULL, or is pointing to a pmalloc'd block

- If event is successfully sent, free all of the memory that was used to 
store the event.  If for some reason event is not sent free up the memory and 
return FALSE


Exception Safe?
===============

HCI Event
----------

There are a number of ways in which it can be detected that an
event is corrupt:

- length of message fragment does not match the length field
- length field does not match the length of the PDU
- the event code is unrecognised
- it is a variable length event, and the count does not match the
  length of the PDU.
- It is a SEF event, and the first two fields are wrong (either
  out of range values, or the length does not match).

Some of the command complete events need fields filled in such as 
BDADDR or connection handle. Care is taken to ensure the alignment
is not messed up

Panic can occur when there is an internal error, and wrpdu_type is 
passed a garbage type. This implies some sort of bug in hci_convert,
and is therefore not expected to occur.


HCI Command
-----------

The code to write command host can only fail if the opcode isnt right.
In this case we return FALSE. Any other fault imply that there is 
either a major bug somewhere, or the chip has got itself into a mess.  
The action taken in these circumstances is to panic the chip.


How it works.
=============

The basic idea is to create a description of each PDU.  This
description will contain the types of the data fields in such a way
that we can convert between the unpacked byte stream 
and format structures

The format of the structures is read from the file hci.h.  This file
has not been changed, so the new hci_convert will remain completely
compatible with existing code. A large quantity of code above the 
HCI level appears to rely on the format of these structures.

A small amount of auxiliary information is supplied by a few extra
text files.  These mainly have a few type definitions in, or list the
HCI commands or events.

From this information two important sets of data get genereated.  The
first is a list of all of the individual PDU formats that are used.
Because we do not care about any typedefs that might have been used in
hci.h we reduce everything to its base type.  This has the happy side
effect that a large number of the PDUs now have exactly the same
format.  For each of the PDU formats we store an array of data items.
Each item specifies the type of the data item, and the offset of this
data item from the start of the structure.  Storing the offset will
hopefully aid portability, and does not appreciably increase the size.

The second piece of important data that is generated is a mapping from
HCI command opcode or event opcode to PDU format string.  There is also
information about the return type for this opcode.  

This is all of the information that is needed for a simple command or
event.  We examine the opcode, and lookup the relevant PDU format.  We
can then simply step through the PDU format array, copying and
converting the data as we go (either to or from a buffer).

The complication in all of this is the PDUs that have a more complex
structure.  These all consist of a fixed format header, and then a
variable length array of 'entries'.  In the structures defined in
hci.h, the memory is allocated for these in a very odd way.  There is
a fixed length array of pointers to 'blocks'.  Each block is
pmalloc'd, and can contain one or more 'entries'.

For these special format PDUs, another structure is generated that
stores the format of the header, and the format of the repeated block.
It also stores the size of the 'blocks' and the number of 'entries' per
'block'.  The variable length PDUs can then be dealt with by common
code that understands this more complex format descriptor.

The only PDUs that do not follow this format exactly are the PDUs
containing local names, or the set event filter command. Local name
is handled by treating it as a standard array PDU (like above), but
made up of uint8's.  The Set Event Filter command needs to be handled
separately.



Conclusion.
===========

Hopefully this rewrite of hci_convert will be better received than the
last one.  The goals while writing this have been to reduce code size
and data size (even if this meant compromising on speed), but
concentrating on code size as much as possible.  If there was an option to
change the internal format of the HCI command structures, a larger
saving of code space could probably be made, but this is not possible
(not to mention a large reduction in RAM usage).


Porting from Bluestack to Synergy 
=================================

In general, all files taken from bluestack are prefixed as per synergy bt
file naming conventions

Perl Scripts
------------ 

The following perl scripts have been taken from the bluestack and modified as 
below:
1. Data types have been replaced with synergy bt types
2. Generation of the format to include the common event part to take care of 
padding/alignment issues seen

csr_bt_hci_convert_prim_size.pl
csr_bt_hci_convert_pdufmtlib.pl
csr_bt_hci_convert_layout.pl
csr_bt_hci_convert_gen_pdufmts.pl
csr_bt_hci_convert_flatten.pl
csr_bt_hci_convert_baggagelib.pl

Running the Perl Scripts
------------------------

Input files:
1. hci_prim.h : Has a list of structure definitions for all the HCI 
commands and events events 

2. bluetooth.h : Has structure definitions for the nested strucutes in 
hci_prim.h file above

3. csr_bt_hci_convert_last_defines.txt : Lists all the compile time
flags that are required. This is taken from bluestack team and additionally
updated with new compile time flags.

4. csr_bt_hci_convert_basic_types.str : Contains the definitions for all
the typedefs

5. csr_bt_hci_convert_simple_events.lst : List of all the events that needs to
be auto generated

6. csr_bt_hci_convert_hci_command_info.lst : List of all the commands that needs to
be auto generated

7. csr_bt_hci_convert_extra_type_defines.lst : Contains list of extra strutcure 
definitions

8. csr_bt_hci_convert_basic_types.lst : All the basic data types used by 
synergy bt

9. csr_bt_hci_convert_manual_pdufmt_list.lst : This is the manual list of pdu 
formats hand written covering all the spcecial cases mainly having variable 
length data

Generated Files
---------------

Following are the files that are generated 
1. csr_bt_hci_convert_hcievt_lut.ch : Event code look up table 
2. csr_bt_hci_convert_hcicmd_lut.ch : Command opcode look up table
3. csr_bt_hci_convert_pdufmts.h : Contains declaration for all the formats 
generated
4. csr_bt_hci_convert_pdufmts.c : Contains definition foa all the formats 
generated as part of the look up tables

Supporting Files
----------------

Following files are re-used and modified for data types, and reverse 
the logic to serialise the commands and de-serialise the events

1.  csr_bt_hci_convert_pdufmt_private.h : Internal pdu format definitions
2.  csr_bt_hci_convert_hcievt_private.h : Internal header for event library
3.  csr_bt_hci_convert_hcicmd_private.h : Internal header for command library
4.  csr_bt_hci_convert_pdufmts_array.c : Handwritten structures for array pdus
5.  csr_bt_hci_convert_pdufmts.c : Generated,see above
6.  csr_bt_hci_convert_pdufmt_write.c : Writes data into the buffer
7.  csr_bt_hci_convert_pdufmt_sizeof.c : Calculates octets per structure
8.  csr_bt_hci_convert_pdufmt_read.c : Read the data in the buffer into a struct
9.  csr_bt_hci_convert_hcievt_id_pdufmt.c : Manages the lookup table for events
10. csr_bt_hci_convert_hcievt_cmd_cmplt.c : All the command complete events are 
handled
11. csr_bt_hci_convert_hcievt_array.c : Read up all the array data into a struct
12. csr_bt_hci_convert_hcicmd_sef.c : Handles the Set event filter command
13. csr_bt_hci_convert_hcicmd_id_pdufmt.c : Manages the lookup table for 
commands
14. csr_bt_hci_convert_hcicmd_array.c : Writes array data into buffer
15. csr_bt_hci_convert_pdufmts.c : Generated,see above

Following file replaces the file that was earlier used by synergy bt which
did the role of HCI conversion manually. The interface functions have been
retained to avoid any changes above the hci layer.

1. csr_bt_hci_convert_auto : Caller of the command/event convert functions

Commands to auto generate
-------------------------

1. csr_bt_hci_convert_layout.pl hci_prim.h bluetooth.h > csr_bt_hci_convert_layout.txt

2. perl csr_bt_hci_convert_flatten.pl -c csr_bt_hci_convert_hci_command_info.lst 
                                      -h hci_prim.h
                                      -I csr_bt_hci_convert_last_defines.txt 
                                         csr_bt_hci_convert_layout.txt 
                                         csr_bt_hci_convert_extra_type_defines.lst 
                                         csr_bt_hci_convert_basic_types.str     
                                       > csr_bt_hci_convert_hcicmd_lut.ch

3. perl csr_bt_hci_convert_flatten.pl -e -k csr_bt_hci_convert_simple_events.lst 
                                      -h hci_prim.h 
                                      -I csr_bt_hci_convert_last_defines.txt 
                                         csr_bt_hci_convert_layout.txt 
                                         csr_bt_hci_convert_extra_type_defines.lst 
                                         csr_bt_hci_convert_basic_types.str 
                                       > csr_bt_hci_convert_hcievt_lut.ch                                       

4. perl csr_bt_hci_convert_flatten.pl -c csr_bt_hci_convert_hci_command_info.lst 
                                      -p -h hci_prim.h 
                                      -I csr_bt_hci_convert_last_defines.txt 
                                         csr_bt_hci_convert_layout.txt 
                                         csr_bt_hci_convert_extra_type_defines.lst 
                                         csr_bt_hci_convert_basic_types.str 
                                       > csr_bt_hci_convert_hcicmd_pdufmt_list    

5. perl csr_bt_hci_convert_flatten.pl -e -p -k csr_bt_hci_convert_simple_events.lst 
                                      -h hci_prim.h 
                                      -I csr_bt_hci_convert_last_defines.txt
                                         csr_bt_hci_convert_layout.txt 
                                         csr_bt_hci_convert_extra_type_defines.lst
                                         csr_bt_hci_convert_basic_types.str 
                                       > csr_bt_hci_convert_hcievt_pdufmt_list                                       

6. perl csr_bt_hci_convert_gen_pdufmts.pl -h csr_bt_hci_convert_manual_pdufmt_list 
                                             csr_bt_hci_convert_hcicmd_pdufmt_list 
                                             csr_bt_hci_convert_hcievt_pdufmt_list 
                                           > csr_bt_hci_convert_pdufmts.h     

7. perl csr_bt_hci_convert_gen_pdufmts.pl -c csr_bt_hci_convert_manual_pdufmt_list 
                                             csr_bt_hci_convert_hcicmd_pdufmt_list 
                                             csr_bt_hci_convert_hcievt_pdufmt_list 
                                           > csr_bt_hci_convert_pdufmts.c                                           


