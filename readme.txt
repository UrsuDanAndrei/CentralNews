Nume: Ursu Dan-Andrei
Grupa: 324CA

=== struct format ====================================================

struct format {
	uint16_t len;
	char content[BUFF_SIZE];
};

Am ales aceasta structura pentru reprezentarea mesajelor ce vor fi
trimise/primite de server/subscriber in cadrul comunicatiilor TCP. Pe
langa campul content, care contine informatia utila, am adaugat si
campul len in care se va stoca lungimea totala a mesajului ce va
fi trimis pe canalul de comunicatie:
(2 (numarul de bytes necesari pentru campul len) + dimensiunea
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

De asemenea, am acoperit si cazul particular in care TCP realizeaza
separarea unui mesaj fix intre cei 2 octeti din campul len.

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
- daca se primeste un mesaj de pe socket-ul UDP:
	-- se apeleaza functia process_received_info
	-- se parseaza mesajul din forma primita in forma care se doreste
	trimisa catre client. Dupa completarea field-ului content se
	calculeaza field-ul len si se apeleaza functia
	send_to_all_subscribers
	-- in aceasta functie se verifica daca exista clienti abonati la
	topicul mesajului. (in caz negativ se ignora mesajul)
	-- se utilizeaza structura topic_subs pentru a parcurge toti
	clientii abonati si se trimite mesajul tuturor clientilor care
	sunt online. Pentru clientii ofline abonati cu flag-ul sf setat
	pe 1 se plaseaza o copie a mesajului in campul inbox.
- daca se primeste un mesaj pe socket-ul TCP pe care se "asculta":
	-- se apeleaza functia add_tcp_client
	-- se accepta conexiunea si se dezactiveaza algoritmul NEAGLE
	pentru socket-ul intors
	-- se introduce noul socket in read_fds
	-- se apeleaza functia get_parssed_messages pentru a se salva in
	vectorul msgs mesajele primite de la client. Daca clientul nu
	trimite macar un mesaj cu numele lui, atunci se inchide
	conexiunea si se afiseaza un mesaj de eroare.
	-- daca clientul isi trimite numele se verifica daca este 
	un client cu totul nou sau este un client inregistrat anterior
	care doar se reconecteaza
	-- in primul caz ii se atribuie clinetului un id unic, se asociaza
	socket-ului pe care s-a conectat acest id si se adauga in lista de
	clienti
	-- in cel de-al doilea caz se verifica daca clientul este deja
	conectat pe un alt socket (caz in care se inchide conexiunea pe
	socket-ul curent) sau a redevenit online
	-- daca a redevenit online se marcheaza acest lucru, se updateaza
	socket-ul clientului si i se trimit toate mesajele din inbox pe
	care le-a primit cat a fost offline (se elibereaza de asemnea
	memoria alocata pentru aceste mesaje)
	-- daca pe langa nume, clientul a trimis si niste requst-uri,
	atunci se parcurge restul vectorului msgs si pentru fiecare mesaj
	se apeleaza functia process_tcp_client_request() pe care o voi
	explica mai jos
- daca nu s-au primit mesaje nici pe socket-ul TCP pe care se asculta,
nici pe UDP si nici de la tastatura, inseamna ca s-au primit mesaje
de la unul din socketii TCP pe care este conectat un client:
	-- se apeleaza functia get_parsed_messages
	-- daca functia intoarce valoare 0, insemna ca subscriber-ul a
	intrerupt conexiunea si se inchide socket-ul
	-- daca functia intoarce o valoare nenula atunci se parcurge 
	vectorul de mesaje si pentru fiecare mesaj se apeleaza functia
	process_tcp_client_request()
	-- in process_tcp_client_request se verifica ca formatul mesajului
	sa fie corect, altfel se ignora
	-- daca s-a primit o cerere de subscribe atunci se introduce
	clientul in lista de abonati la topicul respectiv (daca se
	gaseste deja acolo atunci doar se updateaza flag-ul de sf cu care
	s-a abonat)
	-- daca s-a primit o cerere de unsubscribe se elimina clientul din
	lista de subscriberi a topicului (daca topicul este inexistent
	sau clientul nu este abonat la el se ignora cererea si se afiseaza
	un mesaj de eroare)

=== alte mentiui =====================================================

Am comentat toate mesajele de eroare care nu erau conforme cu cerinte,
pentru a le vizualiza se pot decomenta toate std::cout-urile.

Am considerat ca un topic "apare" in program daca cel putin un client
este abonat la el (daca se primea o lista de topicuri valide inainte
se putea evita scenariul in care clientul "umple" server-ul cu
topicuri pentru care nu se vor primi niciodata mesaje)

======================================================================
