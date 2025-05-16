/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,
    JOINREP,
    DUMMYLASTMSGTYPE,
	PING
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
	Address sourceAddr;
	long heartbeat;
	vector<MemberListEntry> membershipList;
} MessageHdr;

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];

	/* Methods for coordination */

	void send_join_rep(Address destinationAddr);
	void ping(Address destinationAddr);

	void join_req_processor(MessageHdr* msg);
	void join_rep_processor(MessageHdr* msg);
	void ping_processor(MessageHdr* msg);

	/* Methods for failure detection */

	bool detect_failure(vector<MemberListEntry>::iterator it);

	/* Methods for membership list management */

	vector<MemberListEntry>::iterator get_from_membership_list(MessageHdr* msg);
	vector<MemberListEntry>::iterator get_from_membership_list(MemberListEntry* toSearch);
	void add_to_membership_list(MemberListEntry e);
	void remove_from_membership_list(vector<MemberListEntry>::iterator it);
	MemberListEntry transform_message_to_member(MessageHdr* e);

	Address to_address(int id, short port);

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */