Three-Phase Commit Application

By: Manuel Perez, Alex King

//////////////////////////
Overview
//////////////////////////

This code is written into two files, coordinator.c and
participant.c. Much of the structure remained the same. Each of
these files still had a main thread and a listening thread. The
coordinator's listening thread was listening for the VOTE_COMMIT
and VOTE_ABORT messages. The participant was listening for the
VOTE_REQUEST, GLOBAL_COMMIT, GLOBAL_ABORT, and DECISION_REQUEST
messages. Since there was no overlap between the messages the
coordinator listens for and the messages the participant listens
for, all messages could be multicasted as they would just be
ignored if they weren’t intended for the receiver. These two
programs work together to test five different test cases to
assure the principles of a three-phase commit protocol are
covered. A repeated iteration with changing factors is used to
test the coordinator and the participant's behaviours. Based on
the iteration, the programs control if a message should be sent,
and which message. For example, on iteration two the participants
send VOTE_ABORT rather than VOTE_COMMIT to exercise that use case.
To implement timeouts, a loop was used which would check if a
message had been received and then sleep for 1 second. If no
message was received when the loop finished, a timeout was
produced.

//////////////////////////
Test Cases
//////////////////////////

Case 1:
The first case had the two participants sending commits to the
coordinator and the coordinator returning a global commit without
any problems. This was done through several not nested loops that
would check for the messages sent in both sides of the system.
Since the participants and the coordinator have listening threads.
It all starts with the coordinator sending request to all
participants. The participants' listening threads see the message
and save it in a variable. The main code of the participants sees
this and sends a commit to the listening thread of the
coordinator. The thread forwards the message to the  main code of
the coordinator that in response sends a global commit. The
participants see the global commit through the listening threads
and forward it to the main code. The main code of the participant
proceeds to find the case of a global commit message and commits.


Case 2:
In the second case one of the participants responds with a global
abort. to test this we had one participant determine from it’s
port (if it was even or odd) to see if it would abort or commit.
If it aborted it would send an abort message to the coordinator
instead of a commit. The coordinator would see the message, send
it to the main body and proceed to send a global abort to the
participants. The participants see the global abort message and
abort themselves.

Case 3:
This case was implemented in a similar way to the second case.
One of the participants takes too long to reply. The coordinator
times out and sends a global abort. The participants see the
global abort and respond by aborting themselves just as in case 2.

Case 4:
In this case, the coordinator fails to send a VOTE_REQUEST message
to the participants. The participants both time out, so they send
VOTE_ABORT messages to the coordinator. When the coordinator
receives these messages, he issues a GLOBAL_ABORT message. The
participants locally abort upon receiving the message.


Case 5:
This case was designed to simulate a DECISION_REQUEST message.
The coordinator sends a VOTE_REQUEST message, and both
participants reply with VOTE_COMMIT. The coordinator then fails to
send a reply to everyone. We made this happen by sending messages
to all of the participants except for one. One of the participants
received a GLOBAL_COMMIT message and one received nothing. The one
that did not receive a message timed out, and sent out a
DECISION_REQUEST message. When the other participant received the
DECISION_REQUEST message, it went into its local log and saw
GLOBAL_COMMIT so it sent that back to the other participant. The
other participant then committed as well. Since the local log was
only needed for the most recent value received, a single variable
could be used to store that information.

