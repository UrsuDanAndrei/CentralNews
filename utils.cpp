#include "utils.h"

int get_parsed_messages(int sockfd, std::vector<std::string>& msgs) {
    int ret_code;

    /* variabila utilaza pentru a marca cazul in care caracterul '\0' (finalul
    unui mesaj) nu a fost citit in buffer-ul anterior
    si trebuie citit in buffer-ul curent */
    static bool trail_0 = false;

    char buffer[BUFF_SIZE + 5];
    memset(buffer, 0, sizeof(buffer));

    /* s-au primit date pe unul din socketii de client,
    asa ca serverul trebuie sa le receptioneze */
    ret_code = recv(sockfd, buffer, BUFF_SIZE - 1, 0);
    DIE(ret_code < 0, "recv");
    int recv_info_size = ret_code;

    if (ret_code == 0) {
        // clientul s-a deconectat
        return -1;
    }

    int msg_offset = 0;
    format* msg;

    /* se separa mesajele unite de TCP in buffer si daca un mesaj nu este primit
    in intregime se citeste de pe socket pana la completarea lui */

    while (msg_offset < recv_info_size) {
        /* trateaza cazul in care caracterul '\0' (finalul unui mesaj) nu a fost
        citit in buffer-ul anterior si trebuie citit in buffer-ul curent */
        if (trail_0) {
            memcpy(buffer, buffer + 1, sizeof(buffer) - 1);
            --recv_info_size;
            trail_0 = false;

            /* trateaza cazul in care '\0' este singurul
            caracter citit in buffer-ul curent */
            if (recv_info_size == 0) {
                break;
            }
        }

        msg = (format *) (buffer + msg_offset);
        uint16_t len = 0;

        /* trateaza cazul in care separarea s-a facut exact intre cei
        2 octeti care definesc dimensiunea mesajului urmator */
        if (msg_offset == recv_info_size - 1) {
            // se plaseaza primul byte
            memcpy(&len, (buffer + msg_offset), 1);

            // se citeste de pe socket
            memset(buffer, 0, sizeof(buffer));
            ret_code = recv(sockfd, (buffer + 1), BUFF_SIZE - 1, 0);
            DIE(ret_code < 0, "recv");
            recv_info_size = ret_code + 1;

            // se plaseaza al doilea byte
            memcpy((((char*)(&len)) + 1), (buffer + 1), 1);

            // se reseteaza offset-ul si pointer-ul catre buffer
            msg = (format *) buffer;
            msg_offset = 0;
        } else {
            len = msg->len;
        }

        int read_len = strlen(msg->content);
        std::string str_msg(msg->content);

        /* daca mesajul a fost continut in intregime in buffer se continua cu
        identificarea si separarea urmatorului mesaj */
        if (read_len == len - 3) {
            msgs.push_back(str_msg);
            msg_offset += len;
            continue;
        }

        /* daca mesajul nu a fost continut in intregime in buffer se realizeaza
        citiri de pe socket pana cand mesajul este integru */
        while (read_len != len - 3) {
            // se citeste un nou buffer
            memset(buffer, 0, sizeof(buffer));

            ret_code = recv(sockfd, buffer, BUFF_SIZE - 1, 0);
            DIE(ret_code < 0, "recv");
            recv_info_size = ret_code;

            str_msg = str_msg + std::string(buffer);
            read_len += strlen(buffer);

            // se seteaza offset-ul in noul buffer citit
            msg_offset = strlen(buffer) + 1;
        }

        msgs.push_back(str_msg);
    }

    // update trail_0
    if (msg_offset > recv_info_size) {
        trail_0 = true;
    } else {
        trail_0 = false;
    }

    return msgs.size();
}
