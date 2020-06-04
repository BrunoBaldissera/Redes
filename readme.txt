LOGIN = admin
SENHA = admin

TRABALHO DE REDES - SCC0142 1º Semestre 2020
Parte 2

BRUNO BALDISSERA - 10724351
BRUNO GAZONI - 7585037
MATHEUS STEIGENBERG POPULIM -  10734710

Programa desenvolvido em versões debian based do Linux (debian, mint e ubuntu), utilizando GCC 7.5.0.

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

Comandos a serem mandados no terminal pelo cliente:
/connect 			Conecta ao server no início do programa
/ping 				Pinga o server, e o server envia a mensagem "pong" de volta
/quit				Sai do programa
/exit				Sai do programa
Ctrl+D ou 'EOF'		Sai do programa
