# SSC0641-file-syncer
**Trabalho de Redes de Computadores (SSC0641)**

Ivan Roberto Wagner Pancheniak Filho

## Introdução

O objetivo desse programa é proporcionar um serviço de transferencia de arquivos por demanda entre diversas máquinas.
Entre as máquinas, as conexões feitas seguem um modelo P2P por TCP. E numa mesma maquina, o usuário faz requisições
para o serviço por _Unix Domain Sockets_, como se fosse um _Daemon_.

## Sobre o Programa

O programa foi desenvolvido no kernel do Linux 6.10.9-zen1, na distribuição NixOS (unstable), com a versão 13.3.0 do compilador `gcc`.
```zsh
 ❯ fastfetch -s OS:Kernel -l none
OS: NixOS 24.11.20240920.a1d9266 (Vicuna) x86_64
Kernel: Linux 6.10.9-zen1
 ❯ gcc --version
gcc (GCC) 13.3.0
Copyright (C) 2023 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

A partir do código-fonte, são gerados dois executáveis, o que faz a transferencia de arquivos e aceita pedidos dos usuários em uma máquina (o _daemon_),
e o que permite com que um usuário interaja com o _deamon_ por IPC (o _client_).

### Video de Demonstracao
[![Video](https://img.youtube.com/vi/Kfxpb1e634c/0.jpg)](https://www.youtube.com/watch?v=Kfxpb1e634c)

### Como Compilar

Para compilar, será necessário ter a ferramenta de terminal `make`. Com ela, rode o seguinte comando dentro do início da pasta principal do repositório:
```bash
make
```
Se a compilação não falhar, os binários `daemon` e `client` irão aparecer na pasta `out`.

### Como Rodar os Binários

#### Daemon

O `daemon` é o principal binário do programa, providenciando os serviços de transferência de arquivos para os usuários da própria e de outras máquinas.
Ele é feito para rodar em segundo plano, e qualquer interação com ele é feito pelo programa `client`.

A chamada genérica de _daemon_ se parece com
```bash
./daemon <diretorio> <porta>
./daemon . 8080 # Exemplo de chamada com <diretorio>=. e <porta>=8080
```
`<diretorio>` é um diretório do qual o `daemon` vai ler os arquivos e fazer as transferências de seus conteúdos, conforme solicitado. 
O caminho deve ser dado de forma relativa ao local de execução do binário, e o `daemon` não lê de forma recursiva pastas que fossam estar no diretório a ele passado.

`porta` é a porta na qual o servidor estará escutando por conexões feitas pelos `daemon`s de outras máquinas.

#### Client

O `client` é a maneira com que requisições de transferência de arquivo são feitas para a sua máquina ou para a máquina de outro na rede.
Os argumentos passados para o executável são comandos repassados para o `daemon`, o qual retorna a sua saída na saída do `client`.
Os comandos possíveis no `client` são:
```bash
./client close # fecha o processo do daemon
./client list <ip> <porta>
./client list 127.0.0.1 8080 # Exemplo de chamada com <ip>=127.0.0.1 (localhost) e <porta>=8080
./client cat <arquivo> <ip> <porta> 
./client cat readme.md 127.0.0.1 8080 # Exemplo de chamada com <arquivo>=readme.md <ip>=127.0.0.1 (localhost) e <porta>=8080
```

`./client close` pede para que o processo do `daemon` rodando na máquina do usuário pare de rodar.

`./client list` lista os arquivos disponíveis para transferência do IP em `<ip>` na porta `<porta>`. `<ip>` deve ser dado em IPv4.

`./client cat` transfere o conteúdo do arquivo em `<arquivo>` do IP em `<ip>` na porta `<porta>` para a saída padrão do programa.

**Atenção!** Caso seja colocado argumentos de mais no `client`, ele trava por esperar que o servidor vá ler esses argumentos, mesmo que o servidor já tenha fechado a conexão com `client`. Isso ocorre pelo IPC ocorrer por um arquivo de socket de _Unix Domain Sockets_.

### Possiveis problemas

#### Não consigo conectar mesmo estando em uma rede local

Confira se os ports TCP que você esta usando estão habilitados para receberem conexões no firewall de sua máquina.

#### O _daemon_ não faz _bind_ com a porta que eu solicitei

Isso ocorre quando o `daemon` não é fechado de forma limpa, espere alguns minutos ou tente outra porta.

### Bugs Conhecidos

- Quando o `client` falha em passar os dados para frente em um _pipe_, o erro de conexão pode fazer com que o `deamon` crashe.






