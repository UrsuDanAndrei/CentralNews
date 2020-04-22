#include "utils.h"

int get_parsed_messages(int sockfd, std::vector<std::string>& msgs) {
    printf("in get_parsed_messages:\n");
    int ret_code;
    
    char buffer[BUFF_SIZE + 5];
    memset(buffer, 0, sizeof(buffer));

    // s-au primit date pe unul din socketii de client,
    // asa ca serverul trebuie sa le receptioneze
    ret_code = recv(sockfd, buffer, BUFF_SIZE, 0);
    DIE(ret_code < 0, "recv");
    int recv_info_size = ret_code;

    if (ret_code == 0) {
        // clientul s-a deconectat
        return 0;
    }

    int msg_offset = 0;
    format* msg;

    while (msg_offset < recv_info_size) {
        msg = (format *) (buffer + msg_offset);
        int len = msg->len;
        int read_len = strlen(msg->content);
        std::string str_msg(msg->content);

        if (read_len == len - 5) {
            msgs.push_back(str_msg);
            msg_offset += msg->len;
            continue;
        }

        while (read_len != len - 5) {
            memset(buffer, 0, sizeof(buffer));
            ret_code = recv(sockfd, buffer, BUFF_SIZE, 0);
            DIE(ret_code < 0, "recv");

            recv_info_size = ret_code;

            str_msg = str_msg + std::string(buffer);
            read_len += strlen(buffer);

            msg_offset = strlen(buffer) + 1;
        }

        msgs.push_back(str_msg);
    }

    return msgs.size();
}
