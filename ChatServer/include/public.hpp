#ifndef PUBLIC_H
#define PUBLIC_H

enum MsgType
{
    DEFAULT_MSG,

    LOGIN_MSG,
    LOGIN_ACK_MSG,
    LOGINOUT_MSG,

    REG_MSG,
    REG_ACK_MSG,
    
    ONE_CHAT_MSG,
    ADD_FRIEND_MSG,

    CREATE_GROUP_MSG,
    ADD_GROUP_MSG,
    GROUP_CHAT_MSG,
    
};

#endif 