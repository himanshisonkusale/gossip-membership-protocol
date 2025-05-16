/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        
        // Create JOINREQ message
        msg = new MessageHdr();
        msg->msgType = JOINREQ;
        msg->sourceAddr = memberNode->addr;

#ifdef DEBUGLOG
        // sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, sizeof(MessageHdr));

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
   return 1;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	MessageHdr* msg = (MessageHdr*) data;

    if(msg->msgType == JOINREQ){
        join_req_processor(msg);   
    }else if(msg->msgType == JOINREP){
        join_rep_processor(msg);
    }else if(msg->msgType == PING){
        ping_processor(msg);
    }
    return true;
}

void MP1Node::join_req_processor(MessageHdr* msg) {
    add_to_membership_list(transform_message_to_member(msg));
    send_join_rep(msg->sourceAddr);
}

void MP1Node::send_join_rep(Address destinationAddr) {
    MessageHdr* msg = new MessageHdr();
    msg->msgType = JOINREP;
    msg->sourceAddr = memberNode->addr;
    emulNet->ENsend(&memberNode->addr, &destinationAddr, (char*)msg, sizeof(MessageHdr));    
    
    free(msg);
}

void MP1Node::join_rep_processor(MessageHdr* msg) {
    add_to_membership_list(transform_message_to_member(msg));
    memberNode->inGroup = true;
}

void MP1Node::ping_processor(MessageHdr* msg) {

    for(int i=0; i < msg->membershipList.size(); i++){
        vector<MemberListEntry>::iterator it = get_from_membership_list(&msg->membershipList[i]);

        if (it != memberNode->memberList.end()) {

            if(msg->sourceAddr.getAddress() == to_address(it->id, it->port).getAddress()){
                it->setheartbeat(msg->membershipList[i].heartbeat);
                it->settimestamp(msg->membershipList[i].timestamp);
            }

            if(msg->membershipList[i].heartbeat > it->getheartbeat()){
                it->setheartbeat(msg->membershipList[i].heartbeat);
                it->settimestamp(par->getcurrtime());
            }

        } else {
            add_to_membership_list(msg->membershipList[i]);
        }
    }
}


/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

    if (memberNode->pingCounter == 0) {

        memberNode->heartbeat++;
        memberNode->memberList[0].heartbeat++;
        memberNode->memberList[0].timestamp = par->getcurrtime();

        for (int i = 1; i < memberNode->memberList.size(); i++) {
            Address address = to_address(memberNode->memberList[i].id, memberNode->memberList[i].port);
            ping(address);
        }
        memberNode->pingCounter = TFAIL;
    } else {
        memberNode->pingCounter--;
    }

    for (vector<MemberListEntry>::iterator iter = memberNode->memberList.begin() + 1; iter != memberNode->memberList.end(); iter++) {
        if(detect_failure(iter)) {
            remove_from_membership_list(iter);
            iter--;
        }
    }

    return;
}

bool MP1Node::detect_failure(vector<MemberListEntry>::iterator it) {

    if (par->getcurrtime() - it->gettimestamp() > TREMOVE) {
        return true;
    }

    return false;
}

void MP1Node::ping(Address destinationAddr) {
    MessageHdr* msg = new MessageHdr();
    msg->msgType = PING;
    msg->membershipList = memberNode->memberList; // piggyback membership list
    msg->sourceAddr = memberNode->addr;
    msg->heartbeat = memberNode->heartbeat;
    emulNet->ENsend( &memberNode->addr, &destinationAddr, (char*) msg, sizeof(MessageHdr));    
    
    free(msg);
}

vector<MemberListEntry>::iterator MP1Node::get_from_membership_list(MessageHdr* msg) {

    return std::find_if(memberNode->memberList.begin(), memberNode->memberList.end(), [&msg](const MemberListEntry& x) 
    { 
        int id = 0;
        short port = 0;
        memcpy(&id, &msg->sourceAddr.addr[0], sizeof(int));
		memcpy(&port, &msg->sourceAddr.addr[4], sizeof(short));
        return x.id == id && x.port == port;
    });
}

vector<MemberListEntry>::iterator MP1Node::get_from_membership_list(MemberListEntry* toSearch) {

    return std::find_if(memberNode->memberList.begin(), memberNode->memberList.end(), [&toSearch](const MemberListEntry& x) 
    { return x.id == toSearch->getid() && x.port == toSearch->getport();});
}

MemberListEntry MP1Node::transform_message_to_member(MessageHdr* e) {
    MemberListEntry member;
    member.id = 0;
    member.port = 0;
    memcpy(&member.id, &e->sourceAddr.addr[0], sizeof(int));
    memcpy(&member.port, &e->sourceAddr.addr[4], sizeof(short));
    member.heartbeat = e->heartbeat;
    return member;
}

void MP1Node::add_to_membership_list(MemberListEntry e) {

    Address addr = to_address(e.getid(), e.getport());
    if (addr == memberNode->addr) {
        return;
    }

    e.timestamp = par->getcurrtime();
    log->logNodeAdd(&memberNode->addr, &addr);
    memberNode->memberList.push_back(e);
}

void MP1Node::remove_from_membership_list(vector<MemberListEntry>::iterator it) {
    Address addr = to_address(it->id, it->port);
    log->logNodeRemove(&memberNode->addr, &addr);
    memberNode->memberList.erase(it);
}

Address MP1Node::to_address(int id, short port) {
    Address address;
    memcpy(address.addr, &id, sizeof(int));
    memcpy(&address.addr[4], &port, sizeof(short));
    return address;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
    int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);
    MemberListEntry myself = MemberListEntry(id, port, memberNode->heartbeat, par->getcurrtime());
    memberNode->memberList.push_back(myself);
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}