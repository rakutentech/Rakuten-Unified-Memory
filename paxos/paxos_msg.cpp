#include "paxos_msg.h"

#include <cstdint>
#include <cstring>

//#define PAXMSG_DEBUG
#ifdef PAXMSG_DEBUG
#define PAXMSG_PRINTF(...) PRINTF(__VA_ARGS__)
#else
#define PAXMSG_PRINTF(...)
#endif

//---------------------------------------------------------
PAXOS_MSG::PAXOS_MSG(){
    _type               = prepare_enum;
    _from_uid               = NULL;
    _proposer_uid           = NULL;
    _promised_proposal_id   = NULL;
    _proposal_id            = NULL;;
    _last_accepted_id       = NULL;;
    _proposal_value         = NULL;
    _last_accepted_value    = NULL;
    _v                      = NULL;
}
//---------------------------------------------------------
PAXOS_MSG::PAXOS_MSG(const PAXOS_MSG& orig){
    _type                   = orig._type;
    _from_uid               = orig._from_uid;
    _proposer_uid           = orig._proposer_uid;
    _promised_proposal_id   = orig._promised_proposal_id;
    _proposal_id            = orig._proposal_id;
    _last_accepted_id       = orig._last_accepted_id;
    _proposal_value         = orig._proposal_value;
    _last_accepted_value    = orig._last_accepted_value;
    _v                      = orig._v;
    _instance_number        = orig._instance_number;
}
//---------------------------------------------------------
PAXOS_MSG::~PAXOS_MSG(){
    //TODO("Think about hot the demarshalled data should be deleted\n");
    /*
    delete _from_uid;
    delete _proposer_uid;
    delete _promised_proposal_id;
    delete _proposal_id;
    delete _last_accepted_id;
    delete _proposal_value;
    delete _last_accepted_value;
    delete _v;
    
    _from_uid               = NULL;
    _proposer_uid           = NULL;
    _promised_proposal_id   = NULL;
    _proposal_id            = NULL;
    _last_accepted_id       = NULL;
    _proposal_value         = NULL;
    _last_accepted_value    = NULL;
    _v                      = NULL;
    */
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::propose(std::shared_ptr<Value> v){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type                = propose_enum;
    m->_proposal_value      = v;

    return m;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::prepare(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<PAXOS_ID> proposal_id){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type 		= prepare_enum;
    m->_from_uid        = from;
    m->_proposal_id	= proposal_id;
    m->_instance_number = instance_number;

	return m;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::nack(std::shared_ptr<Node> from, std::shared_ptr<Node> proposer_uid, size_t instance_number,std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<PAXOS_ID> promised_proposal_id){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type			= nack_enum;
    m->_from_uid        = from;
    m->_proposer_uid    = proposer_uid;
    m->_proposal_id	= proposal_id;
    m->_promised_proposal_id         = promised_proposal_id;
    m->_instance_number         = instance_number;

    return m;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::promise(std::shared_ptr<Node> from, std::shared_ptr<Node> proposer_uid, size_t instance_num, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<PAXOS_ID> last_accepted_id, std::shared_ptr<Value> last_accepted_val){
    PAXMSG_PRINTF("PAXOS_MSG::promise\n");
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type                = promise_enum;
    m->_from_uid        = from;
    m->_proposer_uid    = proposer_uid;
    m->_instance_number     = instance_num;
    m->_proposal_id	= proposal_id;
    m->_last_accepted_id    = last_accepted_id;
    m->_last_accepted_value = last_accepted_val;

    return m;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::accept(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> proposal_value){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type 		= accept_req_enum;
    m->_from_uid        = from;
    m->_proposal_id	= proposal_id;
    m->_instance_number     = instance_number;
    m->_proposal_value    = proposal_value;

    return m;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::accepted(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> v){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type 		= accepted_enum;
    m->_from_uid        = from;
    m->_proposal_id	= proposal_id;
    m->_instance_number     = instance_number;
    m->_proposal_value    = v;
    return m;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::resolution(std::shared_ptr<Node> from, std::shared_ptr<Value> v){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type = resolution_enum;
    m->_from_uid        = from;
    m->_v    = v;

    return m;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::sync(std::shared_ptr<Node> from, size_t instance_number){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type = sync_enum;
    m->_from_uid        = from;
    m->_instance_number = instance_number;

    return m;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::catchup(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<Value> v){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();

    m->_type = catchup_enum;
    m->_from_uid        = from;
    m->_instance_number = instance_number;
    m->_v    = v;

    return m;
}
//---------------------------------------------------------
size_t PAXOS_MSG::marshal_len(){
    size_t len = 0;
    
    len += sizeof(MSG_ENUM_TYPE);                   // type
    len += Node::marshal(_from_uid, NULL, 0, false);      


    switch(_type){
        case prepare_enum:
            len += PAXOS_ID::marshal(_proposal_id, NULL, 0, false);
            len += sizeof(size_t);                          // instance_number
            break;
        case promise_enum:
            len +=  Node::marshal(_proposer_uid, NULL, 0, false);
            len += PAXOS_ID::marshal(_proposal_id, NULL, 0, false);
            len += PAXOS_ID::marshal(_last_accepted_id, NULL, 0, false);
            len += Value::marshal(_last_accepted_value, NULL, 0, false);
            len += sizeof(size_t);                          // instance_number
            break;
        case accept_req_enum:
            len += PAXOS_ID::marshal(_proposal_id, NULL, 0, false);
            len += Value::marshal(_proposal_value, NULL, 0, false);
            len += sizeof(size_t);                          // instance_number
            break;
        case accepted_enum:
            len += PAXOS_ID::marshal(_proposal_id, NULL, 0, false);
            len += Value::marshal(_proposal_value, NULL, 0, false);
            len += sizeof(size_t);                          // instance_number
            break;
        case resolution_enum:
            len += Value::marshal(_v, NULL, 0, false);
            break;
        case propose_enum:
            len += Value::marshal(_proposal_value, NULL, 0, false);
            break;
        case sync_enum:
            len += sizeof(size_t);                          // instance_number
            break;
        case catchup_enum:
            len += Value::marshal(_v, NULL, 0, false);
            len += sizeof(size_t);                          // instance_number
            break;
        case nack_enum:
            len += Node::marshal(_proposer_uid, NULL, 0, false);
            len += PAXOS_ID::marshal(_proposal_id, NULL, 0, false);
            len += PAXOS_ID::marshal(_promised_proposal_id, NULL, 0, false);
            len += sizeof(size_t);                          // instance_number
            break;
        default:
            TODO("PAXOS_MSG::marshal_len: not yet impl %s\n", enum_name[_type].c_str());
    }
    return len;
}
//---------------------------------------------------------
size_t PAXOS_MSG::marshal(char* buf, size_t len){
    size_t pos = 0;
    
    // _type
    std::memcpy(buf, &_type, sizeof(MSG_ENUM_TYPE));
    pos += sizeof(MSG_ENUM_TYPE);
    
    // _from_uid
    pos += Node::marshal(_from_uid, buf + pos, len, true);

    PAXMSG_PRINTF("PAXOS_MSG::marshal : %s (%d)\n", enum_name[_type].c_str(), _type);
    PAXMSG_PRINTF("PAXOS_MSG::marshal : pos %ld\n", pos);
    switch(_type){
        case prepare_enum:
            pos += ::PAXOS_ID::marshal(_proposal_id, buf + pos, len, true);
            // instance_number
            std::memcpy(buf + pos, &_instance_number, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case promise_enum:
            pos += Node::marshal(_proposer_uid, buf + pos, len, true);
            pos += PAXOS_ID::marshal(_proposal_id, buf + pos, len, true);
            pos += PAXOS_ID::marshal(_last_accepted_id, buf + pos, len, true);
            pos += Value::marshal(_last_accepted_value, buf + pos, len, true);
            // instance_number
            std::memcpy(buf + pos, &_instance_number, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case accept_req_enum:
            pos += PAXOS_ID::marshal(_proposal_id, buf + pos, len, true);
            pos += Value::marshal(_proposal_value, buf + pos, len, true);
            // instance_number
            std::memcpy(buf + pos, &_instance_number, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case accepted_enum:
            pos += PAXOS_ID::marshal(_proposal_id, buf + pos, len, true);
            pos += Value::marshal(_proposal_value, buf + pos, len, true);
            // instance_number
            std::memcpy(buf + pos, &_instance_number, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case resolution_enum:
            pos += Value::marshal(_v, buf + pos, len, true);
            break;
        case propose_enum:
            pos += Value::marshal(_proposal_value, buf + pos, len, true);
            break;
        case sync_enum:
            // instance_number
            std::memcpy(buf + pos, &_instance_number, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case catchup_enum:
            pos +=  Value::marshal(_v, buf + pos, len, true);       // Should this be the proposal_value???
            // instance_number
            std::memcpy(buf + pos, &_instance_number, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case nack_enum:
            pos += Node::marshal(_proposer_uid, buf + pos, len, true);
            pos += PAXOS_ID::marshal(_proposal_id, buf + pos, len, true);
            pos += PAXOS_ID::marshal(_promised_proposal_id, buf + pos, len, true);
            // instance_number
            std::memcpy(buf + pos, &_instance_number, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        default:
            TODO("PAXOS_MSG::marshal: not yet impl %s\n", enum_name[_type].c_str());
    }
    
    //for(auto i = 0 ; i < pos ; i++){
    //    printf("PAXOS_MSG::marshal [%d] %c\n", i, *(buf + i));
    //}
    return pos;
}
//---------------------------------------------------------
std::shared_ptr<PAXOS_MSG> PAXOS_MSG::demarshal(char* buf, size_t len){
    std::shared_ptr<PAXOS_MSG> m = std::make_shared<PAXOS_MSG>();
    size_t pos = 0;
    size_t tmp_pos;
    
    // the type
    std::memcpy(&(m->_type), buf, sizeof(MSG_ENUM_TYPE));
    pos += sizeof(MSG_ENUM_TYPE);
    
    PAXMSG_PRINTF("PAXOS_MSG::demarshall: %s\n", enum_name[m->_type].c_str());
    
    // _from_uid
    tmp_pos = 0;
    m->_from_uid = Node::demarshal(buf + pos, len, &tmp_pos);
    pos += tmp_pos;
    
    
    PAXMSG_PRINTF("PAXOS_MSG::demarshal: from_uid:  %s (%ld)\n", m->_from_uid->to_str().c_str(), m->_from_uid->to_str().length());
    PAXMSG_PRINTF("PAXOS_MSG::demarshal: pos %ld\n", pos);
    switch(m->_type){
        case prepare_enum:
            tmp_pos = 0;
            m->_proposal_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
            pos += tmp_pos;
            
            // instance_number
            std::memcpy(&(m->_instance_number), buf + pos, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case promise_enum:
            tmp_pos = 0;
            m->_proposer_uid = Node::demarshal(buf + pos, len, &tmp_pos);
            pos += tmp_pos;
            
            tmp_pos = 0;
            m->_proposal_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
            pos += tmp_pos;

            tmp_pos = 0;
            m->_last_accepted_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
            pos += tmp_pos;

            tmp_pos = 0;
            m->_last_accepted_value = Value::demarshal(buf + pos, len, &tmp_pos);
            pos += tmp_pos;    
            
            // instance_number
            std::memcpy(&(m->_instance_number), buf + pos, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case accept_req_enum:
            tmp_pos = 0;
            m->_proposal_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
            pos += tmp_pos;
            
            tmp_pos = 0;
            m->_proposal_value = Value::demarshal(buf + pos, len, &tmp_pos);
            pos += tmp_pos;  
            
            // instance_number
            std::memcpy(&(m->_instance_number), buf + pos, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case accepted_enum:
            tmp_pos = 0;
            m->_proposal_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
            pos += tmp_pos;
            
            tmp_pos = 0;
            m->_proposal_value = Value::demarshal(buf + pos, len, &tmp_pos);
            pos += tmp_pos;   
            
            // instance_number
            std::memcpy(&(m->_instance_number), buf + pos, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case resolution_enum:
            tmp_pos = 0;
            m->_v = Value::demarshal(buf + pos, len, &tmp_pos);
            pos += tmp_pos;   
            break;
        case propose_enum:
            tmp_pos = 0;
            m->_proposal_value = Value::demarshal(buf + pos, len, &tmp_pos);
            pos += tmp_pos;   
            break;
        case sync_enum:
            // instance_number
            std::memcpy(&(m->_instance_number), buf + pos, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case catchup_enum:
            tmp_pos = 0;
            m->_v = Value::demarshal(buf + pos, len, &tmp_pos);
            pos += tmp_pos;
            
            // instance_number
            std::memcpy(&(m->_instance_number), buf + pos, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        case nack_enum:
            tmp_pos = 0;
            m->_proposer_uid = Node::demarshal(buf + pos, len, &tmp_pos);
            pos += tmp_pos;
            
            tmp_pos = 0;
            m->_proposal_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
            pos += tmp_pos;
            
            tmp_pos = 0;
            m->_promised_proposal_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
            pos += tmp_pos;
            
            // instance_number
            std::memcpy(&(m->_instance_number), buf + pos, sizeof(size_t));
            pos += sizeof(size_t);
            break;
        default:
            TODO("PAXOS_MSG::demarshal: not yet impl %s\n", enum_name[m->_type].c_str());
    }
    return m;
}
//---------------------------------------------------------
//---------------------------------------------------------
void PAXOS_MSG::debug(){
    PRINTF("--------------\n");
    PRINTF("PAXOS_MSG::debug\n");
    PRINTF(" _type = %s\n", enum_name[_type].c_str());
    
    PRINTF("%p\n", (void*) reinterpret_cast<void *>(&_from_uid));
    PRINTF("%p\n", (void*) reinterpret_cast<void *>(&_proposer_uid));
/*
    _promised_proposal_id   = NULL;
    _proposal_id            = NULL;;
    _last_accepted_id       = NULL;;

    _proposal_value         = VALUE_NULL;
    _last_accepted_value    = VALUE_NULL;
    _v                      = VALUE_NULL;
    */
    PRINTF("--------------\n");
}
//---------------------------------------------------------
