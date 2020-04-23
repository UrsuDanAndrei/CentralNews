Nume: Ursu Dan-Andrei
Grupa: 324CA

=== struct format ====================================================

struct format {
	int len;
	char content[BUFF_SIZE];
};

Am ales aceasta structura pentru reprezentarea mesajelor ce vor fi
trimise/primite de server/subscriber in cadrul comunicatiilor TCP. Pe
langa campul content, care contine informatia utila, am adaugat si
campul len in care se va stoca lungimea totala a mesajului ce va
fi trimis pe canalul de comunicatie:
(4 (numarul de bytes necesari pentru campul len) + dimensiunea
informatiei utile ce trebuie trimise + 1 (caracterul '\0' cu care
am ales sa marchez finalul mesajului)).

Am considerat necesara adaugarea campului len deoarece TCP poate
uni/separa informatiile trimise prin apeluri distincte ale functiei
send si cu ajutorul lui len se poate realiza separarea/unirea
informatiilor primite prin functia recv.

Aceasta separeare/unire a mesajelor este realizata in functia:
int get_parsed_messages(int sockfd, std::vector<std::string>& msgs)
din fisierul utils.h. Functia primeste socket-ul de pe care se 
poate realizeaza o citire si returneaza prin intermediul vectorului
msg toate mesajele care au fost citite de pe acel socket (vectorul
contine pentru fiecare mesaj citit de pe socket numai informatia utila
(campul content din structura format)). Functia returneaza valoarea 0
daca clientul s-a deconectat, altfel returneaza lungimea vectorului
msg.

Dupa fiecare citire utilizand functia recv informatie este plasata
intr-un buffer. Pentru a asigura separearea mesajelor se intra intr-un
while care se termina numai cand suma tuturor dimensiunilor mesajelor
(suma campurilor len) devine egala cu numarul de octeti cititi in
buffer.

Pentru a asigura unirea unui mesaj care a fost separat de TCP, se
verifica daca dimensiunea mesajului specificata in campul len
corespunde cu dimensiunea campului contents. Daca nu corespunde atunci
se realizeaza citiri in buffer cu ajutorul functie recv pentru
obtinerea restului mesajului.

=== subscriber.cpp ===================================================

Etapele de executie (incepand din functia main) sunt urmatoarele:

- se verifica daca numarul de argumente primite de program este valid
- se creaza socket-ul TCP si se dezactiveaza algoritmul NEAGLE pe
acest socket
- se apeleaza functia connect pentru a realiza conexiunea socket-ului
cu server-ul al carui parametrii (ip si port) sunt preluati din argv
- se trimite primul