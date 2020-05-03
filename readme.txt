TRABALHO DE REDES - SCC0142 1S2020
Parte 1

BRUNO BALDISSERA - 10724351
BRUNO GAZONI - 7585037
MATHEUS STEIGENBERG POPULIM -  10734710


Programa desenvolvido em versões debian based do Linux (debian, mint e ubuntu), utilizando GCC 7.5.0.

Instruções para compilar e executar:

Compilar servidor: comp_servidor
Compilar cliente: comp_cliente

Executar servidor: roda_servidor
Executar cliente: roda_cliente

Execute o servidor primeiro, em seguida o cliente, e então comecem a trocar mensagens.

Ao executar, a primeira mensagem inserida será usada pelo outro terminal para definir seu nome.

O tamanho máximo da mensagem está definido como 1024, portanto qualquer mensagem maior que isso será quebrada
em mensagens de 1023 caracteres e um '\0'. Não foi possível definir o limite máximo como 4096, devido a limitações
do próprio terminal. Esse tamanho pode ser mudado facilmente pela variável "int msg_max_size"(linha 84 do client.c,
e 98 do server.c).

Comandos a serem mandados no terminal para terminar a conexão:
\exit
\quit
\sair

Comando para "pingar" o outro usuário:
\ping
