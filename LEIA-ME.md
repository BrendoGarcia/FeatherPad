# FeatherPad — launcher nativo leve para Windows

Projeto C++17 / Win32, sem navegador, Python, Java, Electron ou bibliotecas de terceiros em tempo de execucao. Interface em portugues simples. Licenca MIT.

## Estado desta entrega

Codigo-fonte implementado e revisado estaticamente. Este pacote NAO inclui executavel compilado: o ambiente de desenvolvimento desta entrega nao tem SDK/compilador Windows disponivel. Nao foi executado em Windows nem testado com controle fisico. Use as instrucoes abaixo para gerar o executavel e validar. A meta de 20–50 MB de RAM e um objetivo, nao um resultado medido ou uma garantia.

## 1. Gerar o executavel

### Alternativa A: no GitHub, sem instalar compilador no PC

1. Crie um repositorio vazio na sua conta GitHub.
2. Extraia o ZIP. Abra o CMD na pasta FeatherPad que contem CMakeLists.txt.
3. Com Git instalado, execute (substitua a URL pela do seu repositorio):

```cmd
git init
git add .
git commit -m "FeatherPad inicial"
git branch -M main
git remote add origin https://github.com/SEU-USUARIO/featherpad.git
git push -u origin main
```

4. Abra a aba Actions do repositorio. O workflow Build Windows deve executar automaticamente. Tambem pode ser iniciado pelo botao Run workflow.
5. Apos o workflow ficar verde, abra a execucao e baixe o artefato FeatherPad-Windows-x64. E necessario estar conectado ao GitHub para baixar artefatos.
6. Extraia FeatherPad.exe para uma pasta fixa, por exemplo C:\Apps\FeatherPad. Execute com duplo clique.

O workflow fornecido e a propria verificacao de compilacao no Windows; se falhar, envie o log. Use o repositorio com os arquivos na raiz, sem uma pasta FeatherPad extra envolvendo CMakeLists.txt. O executavel nao e assinado digitalmente.

### Alternativa B: compilar localmente

Instale Visual Studio Build Tools com o componente Desenvolvimento para desktop com C++ e o SDK do Windows. Depois abra **x64 Native Tools Command Prompt for VS**, entre nesta pasta e execute:

```cmd
build.cmd
```

Resultado esperado: `dist\FeatherPad.exe`. O runtime C++ e vinculado estaticamente: para executar em outro computador nao precisa instalar Visual Studio. O compilador e necessario apenas para criar o EXE.

Tambem existe suporte a CMake:

```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
```

## 2. Configurar os jogos

1. Clique Novo.
2. Clique Procurar e selecione um .exe ou atalho .lnk/.url.
3. Preencha um nome e clique Salvar.
4. Se o jogo precisar, informe argumentos e pasta de trabalho. O caminho do executavel deve ser informado sem aspas externas. Argumentos podem conter aspas.
5. Selecione um jogo e clique Jogar, pressione Enter ou use o controle.

Tambem pode informar manualmente uma URI como `steam://rungameid/292030`. O aplicativo correspondente precisa estar instalado. Jogos de lojas podem exigir seus clientes; FeatherPad nao elimina essa exigencia. A lista nao e descoberta automaticamente.

Para editar, selecione, altere e Salvar. Para criar outro, clique Novo primeiro. Remover exclui apenas a entrada da lista, apos confirmacao.

## 3. Usar o controle

Pareie o DualShock 4 ou DualSense nas configuracoes de Bluetooth do Windows, ou conecte via USB. O Windows precisa reconhecer um joystick/gamepad HID.

Mapeamento inicial, que pode variar conforme o dispositivo:

- Botao HID 13: abrir/ocultar (normalmente PS).
- Botao HID 2: confirmar/jogar (normalmente X).
- Botao HID 3: voltar/ocultar (normalmente circulo).
- Direcional e eixo Y do analogico esquerdo: selecionar jogo.

Abra Configuracoes > Diagnostico dos botoes. Aperte cada botao e confira os numeros na linha inferior. Para mudar um comando, escolha Configurar botao, solte todos os botoes e pressione o desejado. Esc cancela a captura. Configure tres botoes diferentes. Nesta versao a captura aceita um botao por acao, nao combinacoes.

A abertura funciona com a janela oculta. Confirmar, voltar e navegar so agem quando o launcher esta em primeiro plano. Ao iniciar um jogo, a janela e ocultada; o monitor permanece ativo. Fechar no X ou Esc oculta o launcher. Para encerrar de verdade, use Configuracoes > Encerrar monitor ou o menu do icone na bandeja.

O Windows pode restringir a tomada de foco: se a janela aparecer atras, clique no icone da bandeja. FeatherPad nao intercepta nem bloqueia os botoes para outros programas. Steam Input, DS4Windows, overlays e jogos podem responder simultaneamente ao mesmo botao. Para diagnosticar duplicacao/ausencia de entradas, teste primeiro com Steam e remapeadores fechados. Nao ha suporte XInput dedicado nesta versao; controles apresentados exclusivamente como XInput podem nao ser detectados.

## 4. Iniciar com o Windows

Marque Configuracoes > Iniciar com Windows. O EXE deve permanecer na mesma pasta depois disso. O programa inicia oculto no proximo login. Desmarque para desativar. Nao precisa de administrador.

Para abrir automaticamente ao conectar, marque tambem Abrir ao conectar controle. Por padrao isso fica desligado: conectar apenas deixa o monitor pronto para o botao de abertura.

**O monitor precisa estar rodando para detectar o controle.** Ligar o controle nao inicia um processo que esteja totalmente encerrado. Por isso existe a opcao de inicializacao com Windows. Trata-se de um aplicativo da sessao do usuario, nao de um servico de sistema.

## Dados e privacidade

Biblioteca e configuracoes: `%LOCALAPPDATA%\FeatherPad\library.ini` (Unicode). Nao usa rede, telemetria ou conta. Para fazer backup, encerre o monitor e copie esse arquivo. Biblioteca limitada a 2000 jogos; campos limitados a 8000 caracteres, sem quebras de linha. Evite editar o INI enquanto o programa roda.

Para desinstalar: desmarque Iniciar com Windows, encerre o monitor e apague a pasta do executavel. Se desejar, apague tambem a pasta de dados acima. Jogos e arquivos originais sao preservados.

## Arquitetura e limites de recursos

- Um unico processo e thread de interface, aguardando GetMessage.
- Raw Input recebe notificacoes de joystick/gamepad HID, inclusive em segundo plano.
- Descriptor HID armazenado por dispositivo; botoes e eixos lidos por HidP.
- Sem consultas periodicas de Bluetooth e sem timer de polling. Enquanto um controle transmite entradas, ha processamento desses eventos mesmo com a janela oculta.
- Biblioteca textual, sem imagens, animacoes ou servidor.
- Sem forcar esvaziamento artificial do working set para aparentar menor RAM.
- Janela e controles permanecem alocados quando ocultos para reduzir complexidade e permitir reabertura rapida.

DS4/DualSense sao alvos de compatibilidade, mas a leitura depende do descriptor HID exposto pelo Windows. USB, Bluetooth, replicas, firmware e remapeadores podem produzir resultados diferentes. O diagnostico e necessario antes de afirmar compatibilidade no seu computador. Nao modifica drivers, nao envia comandos ao controle e nao inicializa Bluetooth por conta propria.

## Roteiro de validacao no seu PC

1. Compile e abra. Cadastre primeiro o Bloco de Notas (`C:\Windows\System32\notepad.exe`) como jogo de teste; confirme abertura com Enter.
2. Feche e reabra o monitor: confirme que cadastro e argumentos permanecem.
3. Conecte o controle por USB; use Diagnostico, configure abertura/confirmar/voltar e teste navegacao.
4. Repita por Bluetooth. Desconecte e reconecte, confira o botao de abertura com o launcher oculto.
5. Cadastre um jogo real. Confirme o inicio e a ocultacao da janela; confira que a navegacao nao interfere no jogo.
6. Ative Iniciar com Windows e reinicie a sessao. Confira o icone e abertura pelo controle. Teste tambem Abrir ao conectar.
7. Tente abrir o EXE duas vezes: apenas uma instancia deve permanecer.
8. No Gerenciador de Tarefas, observe FeatherPad.exe apos um minuto com janela aberta, oculta, controle desconectado e jogo rodando. Registre memoria e CPU. Para avaliar o teto, confira tambem tamanho de confirmacao/bytes privados no Monitor de Recursos: working set e memoria privada sao metricas diferentes.
9. Teste falha: caminho inexistente deve exibir erro e manter o launcher aberto.
10. Teste Encerrar monitor: o processo deve desaparecer e deixar de reagir ao controle.

Se ultrapassar 50 MB no seu PC, envie as medicoes, versao do Windows e modo de conexao. Nao ha medicao real nesta entrega.

## Fontes tecnicas

- https://learn.microsoft.com/en-us/windows/win32/inputdev/raw-input
- https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/interpreting-hid-reports
- https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/hidpi/nf-hidpi-hidp_getusagevalue
