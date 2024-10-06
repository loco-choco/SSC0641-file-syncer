# SSC0641-file-syncer
**Trabalho de Redes de Computadores (SSC0641)**

## Introducao

O objetivo desse programa 'e proporcionar um servico de sincronizacao de arquivos por demanda entre diversar maquinas.
Entre as maquinas, as conexoes feitas seguem um modelo P2P por TCP. E dentro de uma mesma maquina, o usuario faz requesicoes
para o servico por _Unix Domain Sockets_, como se fosse um _deamon_.

```mermaid
flowchart LR

A[Maquina A] <--> B[Maquina B]
B <--> C[Maquina C]
A1[Usuario A1] <--> A
A2[Usuario A2] <--> A
B <--> B1[Usuario B1]
C <--> C1[Usuario C1]
```



