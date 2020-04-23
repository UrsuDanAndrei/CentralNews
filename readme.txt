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

(M-am gandit initial la adaugarea unui # la inceputul si finalul
fiecarui mesaj, dar acesta metoda, chiar daca ar fi fost mult mai
simplu de implementat, ar fi restrans cu mult generalitatea
mesajelor trimise, acestea neavand voie sa contina caracterul #)

=== subscriber.cpp ===================================================

Etapele de executie (incepand din functia main) sunt urmatoarele:

- se verifica daca numarul de argumente primite de program este valid
- se creaza socket-ul TCP si se dezactiveaza algoritmul NEAGLE pe
acest socket
- se apeleaza functia connect pentru a realiza conexiunea socket-ului
cu server-ul al carui parametrii (ip si port) sunt preluati din argv
- se trimite primul mesaj ce contine numele subscriber-ului (primit
ca parametru in argv)
- pentru ca citirea de pe socket si citirea de la stdin sa nu se
blocheze reciproc, am adaugat cei doi file descriptori in read_fds
si am utilizat functia select
- functia select se va debloca daca un file descriptor din read_fds
este capabil sa realizeze o citire fara sa se blocheze
- daca s-a primit o comanda de la stdin:
	-- se citeste de la tastatura comanda si se verifica
	corectitudinea ei cu ajutorul functiei:
	check_correct_input_subscriber
	-- daca functia returneaza -1, inseamna ca comanda este invalida
	si este ignorata (se afiseaza si un mesaj de eroare). Daca
	returneaza 0 insemana ca s-a primit comanda exit si se incheie
	executia programului
- daca se poate citi de pe socket-ul catre server se apeleaza functia
get_parsed_messages descrisa mai sus si se afiseaza mesajele primite
de la server. Daca serverul a inchis conexiunea, executia programului
se opreste.

=== structuri de date utilizate ======================================

Am optat pentru unorderd_map/unordered_set in detrimentului lui
map/set deoarece operatiile sunt efectuate mai rapid si elementele din
structurile pe care le utilizez nu necesita pastrarea intr-o
anumita ordine.

std::unordered_set<int> all_sockets:
Contine lista tuturor file descriptorilor activi la un moment dat.
Am ales sa reprezint aceasta lista cu un unordered_set si nu cu un
vector deoarece se pot realiza eliminari si adaugari (operatii ce vor
fi utilizate frecvent in cadrul programului) in O(1).

std::unordered_map<std::string, int> cli2id:
Am considerat utila atribuirea unui id fiecarui client (identificat 
altfel prin nume) care se conecteaza la server pentru a il gestiona
mai usor. Aceasta atribuire se realizeaza doar la prima conectare si
ramane neschimbata pana la incetarea executiei programului.
(indiferent daca clientul este online sau offline sau se reconecteaza)
Atribuirea id-ului se face incremental.

std::unordered_map<std::string, std::unordered_set<int>> topic_subs:
Aceasta structura realizeaza legatura intre topic si lista de
id-urile a subscriberilor abonati la acel topic. Am ales sa reprezint
aceasta lista cu un unordered_set si nu cu un vector deoarece se pot
realiza eliminari si adaugari (operatii ce vor fi utilizate frecvent
in cadrul programului) in O(1).

std::unordered_map<int, int> sockfd2cli:
Realizeaza corespondenta intre socket-ul pe care este conectat un
client si id-ul acestuia.

std::vector<Client> clis:
Contine vectorul cu toti clientii (online sau offline) care s-au
inregistrat vreodata la server.

Client:

int id
bool on
std::string name

int sockfd:
Socket-ul pe care este conectat curent

std::vector<format*> inbox:
Vectorul contine pointeri la toate mesajele de la topicele la care
clientul s-a abonat cu sf = 1 si pe care acesta trebuia sa le
primeasca daca ar fi fost online

std::unordered_map<std::string, bool> topic_sf:
Retine pentru fiecare topic la care este abonat clinetul flag-ul de sf

=== server.cpp =======================================================

Etapele de executie (incepand din functia main) sunt urmatoarele:

- se verifica daca s-a primit numaraul corect de parametrii
- se creaza is se initializeaza socket-ul TCP cu numarul portului
primit ca parametru
- se creaza is se initializeaza socket-ul UDP cu numarul portului
primit ca parametru
- se apeleaza functia listen si se asteapta conexiuni TCP
- se introduc in read_fds citirea standard, socket-ul TCP si socket-ul
UDP pentru a fi multiplexate de functia select
- daca se primeste o comanda de la stdin atunci:
	-- se verifica daca comanda este valida (in caz contrar se
	afiseaza un mesaj de eroare)
	-- daca comanda primita este "exit" atunci se inchid toate
	conexiunile cu server-ul si se inceteaza executia programului
- daca se primeste un mesaj de pe socket-ul TCP:
	--

