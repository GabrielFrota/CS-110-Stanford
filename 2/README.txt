4 otimizações muito importantes foram identificadas.

O código é confuso e repetitivo em várias partes, e tem muitas outras otimizações menores 
possíveis, mas a performance em todos os testes já está em 100% após essas 4.


fileops.c - adicionado o campo inumber, na struct openFileTable. 

Inicialmente um arquivo aberto tinha apenas o seu pathname, e antes de qualquer operação IO tinha 
que traduzir o pathname, percorrendo todos os inodes até o seu inode final. Com o inumber salvo na 
struct, um arquivo precisa fazer o processo de traduçao e busca no disco apenas uma vez. Como 
também foi implementado um cache de inodes no sistema, um inode_get(inumber) de um inode 
importante provavelmente retornará um inode em memória, sem precisar acessar o disco. Pensei em 
salvar na struct openFileTable um ponteiro para o inode em cache, ao invéz do inumber, mas isso 
causaria problemas na hora que o cache está cheio, e é preciso retirar inodes do cache. O inumber 
foi a solução escolhida então.


pathstore.c - adicionado o campo chksum, na struct PathstoreElement. 

Durante o processo de adicionar um PathstoreElement ao indice, é verificado se este 
PathstoreElement não é igual a nenhum PathstoreElement que já está no indice, comparando-o 
atraves da funçao IsSameFile, para evitar arquivos repetidos no indice. Um dos testes consiste em 
comparar os conteudos dos arquivos através de um checksum. Inicialmente, ao adicionar um novo 
arquivo ao indice, todos os arquivos já presentes no indice eram relidos por inteiro novamente, 
para poder computar o checksum, e poder checar se o novo arquivo a ser adicionado não era igual a 
algum já presente no indice.
Após a otimização, um novo PathstoreElement no processo de ser adicionado ao indice, tem seu 
checksum computado pela primeira vez antes de entrar no loop de testes IsSameFile, e caso ele passe 
em todos os testes, é adicionado ao indice com seu valor do checksum na struct, podendo assim 
entrar na comparação de futuros PathstoreElement sem precisar ter seu checksum computado do zero 
todas as vezes. Otimização muito importante para os discos vlarge.img e manyfiles.img, ambos 
travavam meu computador antes dessa otimização. (thrashing provavelmente)


CacheSetor.c e CacheInode.c

implementado um cache de setores do disco e um cache de inodes. A descrição em detalhes do 
funcionamento do cache eu não consigo escrever em um texto direito. Posso explicar em pessoa com
o auxilio de desenhos em um papel.



