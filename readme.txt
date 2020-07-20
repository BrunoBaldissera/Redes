TRABALHO DE REDES - SCC0142 1º Semestre 2020
Parte 3

BRUNO BALDISSERA 	    -  	10724351
BRUNO GAZONI 		    -  	7585037
MATHEUS STEIGENBERG POPULIM -  	10734710

Programa desenvolvido em versões debian based do Linux (debian, mint e ubuntu), utilizando GCC 7.5.0.

AVISO IMPORTANTE:
		A comunicação feita entre redes foi testada utilizando a ferramenta ngrok, 
	em que a máquina que executa o programa servidor deve utilizar para fazer um tunelamento
	com um servidor ngrok externo, para que os clientes por sua vez se conectem ao ip e porta
	providenciados por este serviço no momento em que o servidor o aciona.
		Esta configũração é feita no makefile, na opção roda_cliente, onde o ip
	do servidor ngrok é passado como argumento do programa, bem como a porta necessária.
		Por exemplo, no último teste feito entre os membros do grupo. A máquina que roda
	o servidor localmente usou o comando: ./ngrok tcp 3000
	já que 3000 é a porta onde o servidor escuta. Então o serviço ngrok
	ofereceu um domínio da seguinte forma:
	Forwarding                    tcp://2.tcp.ngrok.io:17353 -> localhost:3000  
		Então foi atualizado no github o makefile para rodar o programa cliente fazendo com que
	o socket escute na porta 17353 do domínio 2.tcp.ngrok.io,
	que por sua vez é identificado pelo ip 3.23.201.37.
		Finalmente, o cliente efetua um pull no github para a versão mais recente, roda o cliente pela
	diretriz do makefile atualizada e a comunicação é estabelecida com servidor entre redes.
		OBS: Por causa dessa dinâmica, o comando /whois acaba por sempre retornar o ip
	do alvo como sendo do localhost, mesmo estando com uma lógica correta de implementação, uma vez
	que a comunicação é interpretada como ocorrendo em uma mesma máquina local.

Instruções para uso em redes distintas:

A máquina que for rodar o servidor deve ter a ferramenta ngrok instalada e disponível.
Esta máquina então usa o comando ./ngrok tcp 3000
onde está instalado o ngrok.
Então o makefile deve ser alterado na linha de roda_cliente,
sendo o primeiro parâmetro o ip obtido após a conversão do domínio que o ngrok oferece no comando anterior
e o segundo parâmetro a porta também oferecida pelo serviço.
Exemplo: tcp://2.tcp.ngrok.io:17353
o ip de 2.tcp.ngrok.io é 3.23.201.37, e a porta é 17353,
logo, no makefile: 
roda_cliente:
	./client 3.23.201.37 17353
E então a máquina compila e executa o servidor via os comandos make compila
e make roda_servidor, enquanto a máquina em outra rede
usa a mesma versão dos arquivos atualizada para apenas rodar o cliente via o comando
make roda_cliente,
e então é guida pela interface de usuário via terminal.

Instruções para compilar e executar:

Compilar servidor e cliente: Executar no terminal "make compila".

Executar servidor: Executar no terminal "make roda_servidor".
Executar cliente: Executar no terminal "make roda_cliente".

Execute o servidor primeiro, em seguida o cliente, e então comecem a trocar mensagens.

Para o Server:
Basta executá-lo. Para encerrar a aplicação, utilize o Ctrl-C.

Para o cliente:
Ao executar, o programa pedirá ao cliente que insira seu nome.
Em seguida, o cliente deverá digitar /connect para se conectar.
Após isso, o cliente deverá digitar login e senha, definidos como "admin" e "admin".
Então, o cliente estará conectado ao servidor, e poderá trocar mensagens com outros clientes.
Como dito na especificação, o Ctrl-C não funciona no lado do cliente, portanto, devem ser
usados os comandos: "/exit", "/quit" ou Ctrl-D (EOF).


Mensagens digitadas pelo cliente com mais de 4096 bytes serão cortadas, e enviadas em partes.

Como o socket foi criado com o protocolo IP, o retorno da função send indica se o cliente recebeu ou não.
São feitas 5 tentativas de envio, e caso nenhuma dê certo, o cliente é desconectado.

Comandos disponiveis: 
	/ping : O servidor responde com "pong".
	/join nomeCanal : Você entrará no canal nomeCanal, e caso ele já não exista, será criado e você será um moderador lá. (Um canal deixa de existir se não houverem mais usuários nele)
	/nickname Apelido : Você mudará o nome pelo qual é reconhecido no chat para Apelido.
Comandos para moderadores: 

	/whois Usuario : O servidor responde com o ip do usuario identificado por Usuario.
	/kick Usuario : Caso ele exista no canal, Usuario é expulso do canal onde o moderador e ele estão.
	/mute Usuario : As mensagens de Usuario não são mais vistas pelos outros usuários, até que ele seja dessilenciado.
	/unmute Usuario : Reverte a condição de silenciamento do Usuario.
	/exit : Sai do IRC e finaliza o programa (é a forma canônica de sair do programa).
