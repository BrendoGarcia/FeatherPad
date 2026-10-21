# FeatherPad 1.1.0 — multiplayer, perfis e biblioteca

## Multiplayer, perfis e biblioteca

- **Multiplayer local:** quando a compatibilidade SDL/ViGEm está ativa, até quatro
  controles reconhecidos podem alimentar saídas virtuais independentes. O Windows
  atribui cada posição XInput disponível; a tela de diagnóstico mostra os dispositivos
  e os índices que o sistema realmente concedeu. No modo nativo, o jogo recebe os
  controles físicos diretamente. A compatibilidade continua sujeita às vagas livres
  no driver virtual e ao suporte do jogo ao formato escolhido.
- **Perfis por controle:** atalhos e zona morta dos analógicos são salvos pela identidade
  do dispositivo (caminho HID, com GUID SDL como alternativa). Em dispositivos que não
  reportam identidade persistente, reconectar por outra porta pode criar um perfil novo.
  A opção Ajustar calibração alterna a zona morta entre níveis predefinidos.
- **RAM e conectividade:** a tela principal informa memória privada e residente do
  FeatherPad, estado de conectividade do Windows e bateria informada pelo controle.
  Alguns modelos/drivers não disponibilizam o nível da bateria; nesses casos aparece
  “n/d”. A conectividade usa o estado de rede do Windows e não testa um site externo.
- **Categorias:** no editor de jogos, informe uma ou mais categorias separadas por
  ponto e vírgula, por exemplo `Corrida; Multiplayer`. O botão Coleções filtra jogos
  por essas categorias e oferece coleções comuns como Corrida, Campanha e Emuladores.
- **Estatísticas:** Detalhes mostra último início e quantidade de aberturas confirmadas.
  O tempo é acompanhado para executáveis `.exe` diretos enquanto o FeatherPad continua
  aberto; atalhos, URIs e launchers externos podem não revelar qual processo é o jogo.

O modo de compatibilidade cria uma saída virtual por jogador conectado, até quatro.
O número XInput depende dos slots disponíveis no Windows; o FeatherPad não consegue
reservar um índice ocupado por outro dispositivo ou emulador.

- **Capas em tela cheia:** recortes arredondados agora usam um caminho GDI que acompanha
  a transformação da tela. Testes verificam pixels em 1366×768, 1920×1080 e 2560×1440.
- **Teclado:** fileiras QWERTY deslocadas, teclas amplas/arredondadas, Shift, apagar,
  espaço largo e Aplicar. O botão 123 alterna números, acentos, símbolos e letras.
  Direcional escolhe pela posição visual; confirmar digita e voltar cancela. O teclado
  físico também digita diretamente, mesmo com uma tecla virtual em foco.
- **Atualizações:** a tag pública e a versão interna deixam de ser comparadas como se
  fossem a mesma numeração. A detecção compara tamanho e SHA-256 dos arquivos reais.
  Assim, uma Release chamada 1.0.1 não é descartada porque o programa foi compilado
  internamente como 1.0.1. Executáveis com versão interna inferior são bloqueados antes
  de aplicar; alterações na mesma versão interna podem ser instaladas após confirmação.

## Releases com ZIP ou arquivos individuais

O launcher prefere os assets individuais, baixando somente os arquivos diferentes.
Também aceita uma Release manual com **FeatherPad-Windows-x64.zip** (ou o ZIP versionado
produzido pelo workflow). Nesse caso oferece baixar o ZIP para inspecionar o conteúdo;
não informa que o aplicativo está atualizado sem conferir os arquivos.

O ZIP é conferido pelo SHA-256 fornecido pela API do GitHub. A extração é nativa, usando
somente o inflador estático da zlib 1.0.1: não abre PowerShell, navegador ou outro programa.
São aceitos ZIP armazenado/Deflate e um pacote na raiz ou dentro de uma pasta. ZIP64,
criptografia, links, caminhos absolutos/ascendentes, nomes duplicados, CRC incorreto e
arquivos fora dos limites são rejeitados. Apenas os arquivos conhecidos são extraídos;
a biblioteca do usuário nunca faz parte do plano de substituição.

O pacote deve conter `FeatherPad.exe`, `Updater.exe`, `SDL3.dll`, `LICENSE`, `LEIA-ME.md`,
`licenses/SDL3.txt` e `licenses/ViGEmClient.txt`. O aviso `licenses/zlib.txt` acompanha os
novos pacotes e também é atualizado quando presente. O ZIP completo tem limite de 64 MB;
cada arquivo descompactado, 128 MB; total extraído, 256 MB. Um ZIP é baixado inteiro —
a economia de download por arquivo só é possível quando a release oferece assets soltos.

A versão do programa agora vem de **um único lugar: `project(... VERSION ...)` no
CMakeLists.txt**. O CMake gera os recursos de versão do EXE e a constante do atualizador.
O script de pacote verifica a versão do binário e exercita a extração do ZIP real. Para
publicação pelo workflow, use uma tag `v` seguida dessa versão. Tags manuais com outra
numeração continuam reconhecidas pelo atualizador, por comparação de conteúdo.

**Migração:** o botão da 1.0.1 continua contendo a lógica antiga. Para receber esta
correção, encerre o monitor e extraia uma vez o pacote completo da 1.0.1. Depois use o
botão corrigido para Releases futuras. Nada é alterado nas Releases já publicadas.

---

## Guia da versão 1.0.1 (recursos preservados; atualização revisada acima)

# FeatherPad 1.0.1 — pesquisa, teclado virtual, tela cheia e atualizações

## Pesquisa pelo controle

Na barra lateral, selecione **Pesquisar jogos**. Também pode usar **F3** ou **Q**.
O teclado virtual abre com o texto atual. Use direcional/analógico para escolher teclas,
confirmar para digitar e **Aplicar** para filtrar a biblioteca. **Voltar/Cancelar** descarta
as alterações. A busca ignora maiúsculas e acentos, combina com Favoritos e encontra
partes do nome. **Limpar pesquisa** volta a mostrar toda a coleção atual.
A busca é temporária e não altera o cadastro dos jogos.

## Teclado virtual nos formulários

Ao navegar pelo controle em um campo editável do cadastro ou perfil, pressione
**Confirmar** para abrir o teclado virtual. Ele oferece letras, números, acentos,
caracteres de caminho, espaço, apagar, maiúsculas, limpar e movimento do cursor.
**Aplicar** transfere o texto ao campo; ainda é necessário salvar o jogo/perfil.
O teclado físico e o mouse continuam disponíveis. Escolher arquivos permanece no
seletor nativo do Windows; é possível digitar o caminho completo pelo teclado virtual.

## Tela cheia para PC e TV

Use **Tela cheia / TV** na barra lateral ou nas configurações; **F11** também alterna.
A biblioteca ocupa o monitor atual sem bordas, escala texto/cartões/ações e preserva
a proporção. Em telas maiores os elementos ficam maiores. Ao sair, restaura o estado
anterior da janela, inclusive maximização. O modo é da sessão; não muda a resolução
do Windows. Esc/Voltar continua ocultando a biblioteca; F11 ou o botão sai da tela cheia.
Diálogos e teclado virtual continuam sendo janelas nativas sobre a biblioteca.

## Atualizar sem baixar novamente o pacote inteiro

**Uma vez:** para migrar de 2.1 ou anterior, extraia o pacote completo de 1.0.1 com
`FeatherPad.exe`, `Updater.exe`, `SDL3.dll`, instruções e licenças. A versão antiga
não possui o botão nem o componente de atualização.

Depois, em **Configurações > Buscar atualizações**:

1. O launcher consulta a última Release estável pública de `BrendoGarcia/FeatherPad`.
2. Compara a versão numérica `vMAJOR.MINOR.PATCH` com a versão instalada.
3. Pergunta se você deseja baixar. Não consulta em segundo plano automaticamente.
4. Compara tamanho e SHA-256 dos arquivos locais com os digests dos assets do GitHub.
5. Baixa somente os arquivos diferentes para `%LOCALAPPDATA%\FeatherPad\updates`.
6. Valida os arquivos e pergunta se pode fechar e reiniciar. Não atualize durante um jogo.
7. Uma cópia do **Updater instalado** aguarda o encerramento, guarda rollback dos arquivos
   anteriores e substitui apenas os arquivos permitidos. O novo Updater baixado não é
   executado para aplicar sua própria atualização.
8. Reinicia o launcher. Biblioteca, capas e preferências não fazem parte da substituição.

O download usa HTTPS com validação TLS do Windows e só aceita os hosts de distribuição
GitHub previstos. A integridade usa os digests SHA-256 fornecidos pela API do GitHub.
**Isso autentica a origem HTTPS/GitHub; não é uma assinatura digital independente do
publicador.** A segurança depende da conta e das Releases oficiais do repositório.
Não há token GitHub embutido: repositório privado, limite de API, falta de internet,
release incompleta ou sem digest resultam em aviso e nenhuma atualização.

A pasta precisa permitir escrita ao usuário. Não há elevação automática nem instalação
silenciosa de drivers. Se a substituição falhar, o atualizador tenta restaurar os arquivos
anteriores. Logs e cópias ficam na pasta da tentativa (`result.txt` e `rollback`). Uma falha
na restauração é informada explicitamente. Essa recuperação não é garantia contra queda
de energia; preserve a pasta de recuperação e seu backup habitual.

Após sucesso, as pastas de recuperação são mantidas para inspeção. Podem ser apagadas
manualmente com o launcher encerrado quando você confirmar que a versão funciona.

### Publicar versões que o botão reconhece

O novo workflow **Release Windows** compila no GitHub, executa os testes e publica um
ZIP completo, checksums e sete assets individuais: `FeatherPad.exe`, `Updater.exe`,
`SDL3.dll`, `LICENSE`, `LEIA-ME.md`, `SDL3.txt` e `ViGEmClient.txt`. Os dois últimos vão
para `licenses` na instalação. Arquivos extras não são instalados automaticamente.

Atualize a versão em `CMakeLists.txt`; o CMake propaga para o EXE e o atualizador. Faça commit e publique uma tag estável correspondente, como
`v2.2.0`. O workflow também aceita execução manual com uma tag já existente. Não
sobrescreve uma release existente. A criação desta funcionalidade não publica uma release.

O usuário não compila nada. O mantenedor continua produzindo os novos binários no
GitHub Actions. O download é incremental **por arquivo**, não um patch binário de partes
de um EXE. Um arquivo alterado é baixado inteiro. A DLL só é baixada se seu hash mudar.

## Testes adicionados em 2.2

A suíte verifica pesquisa com acentos/favoritos/resultado vazio, aplicação e cancelamento
do teclado virtual, edição de campos, entrada/saída da tela cheia, comparação de versões,
parser JSON, allowlist, SHA-256, plano de atualização e restauração com destino bloqueado.
A suíte não publica releases nem acessa a internet. O uso em TV, controle físico e um ciclo
real entre duas releases devem ser verificados em hardware antes da distribuição ampla.

---

# Guia técnico da base 2.1 (histórico e recursos preservados)


Launcher nativo para Windows x64, escrito em C++17 e Win32. Sem navegador, Electron, Java, Python, conta ou conexão com a internet em tempo de execução.

## Compatibilidade de controles nos jogos (2.1)

**Esta versão acrescenta conversão de controles; não é uma promessa de compatibilidade universal.** O controle físico é lido pela SDL e pode aparecer para o jogo como Xbox 360, PlayStation 4 (DualShock 4) ou continuar no modo nativo. Não é necessário manter Steam ou DS4Windows aberto.

### Ativação única

1. Baixe **FeatherPad-Windows-x64** no GitHub Actions e extraia **todos os arquivos**, incluindo `SDL3.dll`, para a pasta do launcher. Encerre o monitor antigo antes de substituir os arquivos.
2. Abra **Configurações > Compatibilidade dos jogos**.
3. Para saídas virtuais, use **Driver necessário** para abrir a página oficial do **ViGEmBus 1.22.0**. Instale o driver uma vez com permissão de administrador; reinicie o Windows se o instalador pedir. O launcher não instala drivers silenciosamente nem baixa executáveis em segundo plano.
4. Volte ao launcher e ative **Ativar compatibilidade de controles**. Se já estava ativado, clique **Verificar novamente** após a instalação.
5. Escolha o formato padrão: Xbox 360, PlayStation 4 ou Nativo. Xbox 360 é o padrão inicial porque oferece uma saída XInput comum em jogos de PC; isso não significa detectar automaticamente a API de todos os jogos.
6. Mantenha **Controle de origem: Automático**, ou selecione um controle reconhecido. Esta versão converte **um controle/jogador por vez**.
7. Confira a mensagem: ela deve indicar o formato virtual ativo e o nome do controle de origem. Inicie o jogo depois disso. Em `joy.cpl`, o controle virtual deve aparecer quando o modo estiver ativo.

A função vem desativada na atualização, preservando o comportamento anterior. A preferência é salva; depois da ativação você não precisa repetir o processo a cada jogo. Em modo Nativo a SDL continua disponível para navegação, mas nenhum dispositivo virtual é criado e o ViGEmBus não é necessário.

### Formatos diferentes por jogo

Todos os jogos antigos e novos começam em **Usar padrão global**. A maioria pode ser aberta assim, sem configuração individual. Se um jogo específico exigir outro formato, use **Editar > Controle para este jogo** e selecione:

| Opção | Comportamento |
|---|---|
| Usar padrão global | Usa o formato escolhido nas configurações do launcher. |
| Nativo — sem conversão | Remove a saída virtual e deixa o jogo usar diretamente o controle físico. |
| Xbox 360 virtual | Converte as entradas do controle reconhecido em XInput. |
| PlayStation 4 virtual | Converte as mesmas entradas em um DualShock 4 virtual. |

A preferência fica salva no cadastro desse jogo e é aplicada antes de abri-lo. O último formato aplicado permanece até o próximo jogo ou uma nova configuração global; o launcher não tenta adivinhar quando clientes externos/atalhos encerram o jogo.

**Não há detecção universal do formato aceito por um executável.** Criar Xbox e PlayStation ao mesmo tempo causaria ambiguidades e entradas duplicadas; esta versão usa somente uma saída virtual por vez. Ela não altera menus internos do jogo, não cria suporte onde o jogo não tem suporte a gamepad e não obriga um jogo a trocar de teclado/mouse para controle.

### Controles reconhecidos

A entrada usa **SDL 3.4.18**, com seus perfis padronizados para controles compatíveis de Xbox, PlayStation, Switch e outros fabricantes, por USB/Bluetooth conforme o dispositivo/driver. Isso amplia o suporte, mas não cobre literalmente todo hardware existente. Controles desconhecidos podem precisar de um mapeamento inicial.

Use **Importar mapeamentos SDL...** para um arquivo `gamecontrollerdb.txt` compatível. Ele é copiado para `%LOCALAPPDATA%\FeatherPad\gamecontrollerdb.txt` e lido na próxima inicialização da camada de controles. A importação não executa código. Um mapeamento descreve o controle físico e vale para todos os jogos; não precisa ser refeito em cada jogo. A lista de origem mostra os dispositivos que a SDL reconhece como gamepads; um joystick sem perfil pode não aparecer até importar um mapeamento adequado.

Com a compatibilidade ativada, a navegação usa botões padronizados: 1=Oeste/quadrado, 2=Sul/X/A, 3=Leste/círculo/B, 4=Norte/triângulo/Y, 5/6=ombros, 7/8=gatilhos, 9=Back/Share, 10=Start/Options, 11/12=cliques dos analógicos, 13=Guide/PS, 14=touchpad. Os números são posições padronizadas, não necessariamente os números HID de uma réplica. Se algum atalho personalizado antigo mudar, configure-o uma vez pelos pop-ups. O botão Guide/PS é reservado para o launcher e não é enviado à saída virtual.

Controles do mesmo modelo podem compartilhar o mesmo GUID; quando há dois iguais, a preferência de origem escolhe o primeiro disponível. Multiplayer e escolha por número de série não estão implementados nesta versão.

### Evitar comandos duplicados

O jogo pode enxergar o controle físico e o virtual simultaneamente. Em jogos que aceitam os dois, isso pode gerar duas entradas. O botão **Evitar entrada dupla** abre a documentação oficial do **HidHide**, um filtro opcional separado. Sua configuração é única por controle:

- Inclua o caminho do `FeatherPad.exe` na lista de aplicativos permitidos do HidHide.
- Selecione apenas o controle **físico** para ocultar dos demais aplicativos e ative a ocultação.
- Deixe o controle virtual visível. Se usar Nativo em algum jogo, desative a ocultação física para esse uso; o launcher não altera automaticamente o HidHide.

Não há ocultação automática neste projeto. Feche Steam Input/DS4Windows durante o primeiro teste para não criar duas camadas de conversão. Dispositivos do barramento ViGEm são excluídos das origens do launcher para impedir realimentação. A opção Nativo não suprime os comandos do dispositivo físico quando a biblioteca está aberta.

### Limites e dependências

- ViGEmBus e ViGEmClient são projetos **aposentados**, sem manutenção futura. A dependência é opcional e explícita. A versão oficial 1.22.0 removeu o atualizador legado; não use antigos atualizadores/domínios. Fontes: [fim de vida](https://docs.nefarius.at/projects/ViGEm/End-of-Life/) e [release oficial](https://github.com/nefarius/ViGEmBus/releases/tag/v1.22.0).
- Esta integração é destinada a **Windows 10/11 x64**. Atualizações do Windows, políticas de segurança, jogos e anti-cheat podem limitar drivers/dispositivos virtuais; não há bypass de anti-cheat ou injeção de DLL no jogo.
- A saída PlayStation é **DualShock 4**, não emulação completa de DualSense. Botões, dois analógicos, gatilhos e direcional são convertidos. Áudio do controle, gyro, touchpad como ponteiro, gatilhos adaptativos, LEDs e retorno de vibração não são encaminhados nesta versão.
- Ao abrir a biblioteca, o controle **virtual** recebe estado neutro. Ao iniciar um jogo, aguarda botões/gatilhos e analógicos voltarem ao repouso antes de encaminhar entradas. O físico só pode ser ocultado por um filtro separado.
- O launcher precisa continuar rodando em segundo plano durante o jogo. Desativar a compatibilidade/encerrar o monitor remove o controle virtual e libera SDL/recursos. Desconectar o controle físico remove o virtual; reconectar recria a saída.
- Caso o driver não esteja pronto, o launcher avisa antes de abrir o jogo. Você pode cancelar para corrigir ou escolher iniciar somente com o controle físico.

### Memória, build e testes

A UI continua nativa Win32. SDL é somente uma biblioteca de entrada carregada sob demanda, não um navegador nem um programa separado. O backend antigo sem timer continua quando a compatibilidade está desativada. Com ela ativada, um timer pede atualização de entradas em aproximadamente 8–16 ms com controle e 250 ms sem controle; isso acrescenta CPU/RAM e precisa ser medido no seu PC. Não há garantia de manter 20–50 MB no novo modo.

O GitHub Actions continua gerando o pacote. O CMake baixa dependências fixas com SHA-256 conferido: SDL 3.4.18 e ViGEmClient no commit `b66d02d57e32cc8595369c53418b843e958649b4`. O cliente ViGEm é vinculado estaticamente; `SDL3.dll` e as licenças são incluídas no artefato. O programa não faz downloads de bibliotecas em tempo de execução.

Os testes verificam conversões, eixos, gatilhos, preferências persistentes e entrada SDL com um gamepad de teste local ao processo, **sem instalar driver na CI**. Isso não valida um jogo real, Bluetooth físico, a instalação do ViGEmBus ou a saída virtual vista por um jogo. Teste no seu PC primeiro com um jogo conhecido e confira o dispositivo em `joy.cpl`. O teste em hardware e a medição de RAM continuam necessários.

## O que mudou nesta versão

- Tema escuro, destaque do jogo selecionado e biblioteca com capas em carrossel.
- Perfil local com nome e imagem, favoritos e coleção filtrada.
- Cadastro e edição em janelas próprias, separados da biblioteca.
- Pop-ups de configuração do controle: ação identificada, espera pela entrada, detecção de conflito e confirmação antes de salvar.
- Diagnóstico em uma janela própria, claramente separado da configuração.
- Capas importadas como miniaturas PNG; cache limitado e liberado ao ocultar.
- Ícone próprio, navegação por controle/teclado e compilação pelo GitHub Actions preservados.

## Atualizar a versão anterior sem perder os jogos

1. Encerre o monitor antigo pelo menu do ícone na bandeja: **Encerrar monitor**. Fechar a janela no X apenas a oculta.
2. Extraia este ZIP. Ele contém a pasta `FeatherPad` com o projeto completo.
3. No repositório que você já usou no GitHub, substitua os arquivos do projeto pelos desta versão. **Inclua os novos arquivos de `src`, a pasta `tests` e `.github/workflows/build.yml`**. Substituir somente `main.cpp` não basta.
4. Mantenha `CMakeLists.txt` e as pastas `src`, `tests` e `.github` na raiz do repositório, sem uma pasta FeatherPad extra envolvendo tudo.
5. Faça commit e push. Abra **Actions > Build Windows** e aguarde os testes e a compilação. Também pode usar **Run workflow** manualmente.
6. Na execução verde, baixe o artefato **FeatherPad-Windows-x64**, extraia e substitua os arquivos do pacote, mantendo SDL3.dll ao lado do executável.
7. Se mantiver a mesma pasta/nome do EXE, a inicialização com Windows continua apontando para o mesmo caminho. Se mudar de pasta, desative e reative essa opção.

O cadastro é lido de `%LOCALAPPDATA%\FeatherPad\library.ini`, o mesmo endereço da versão anterior. Campos antigos são preservados. As capas começam vazias: edite cada jogo para adicioná-las. Na primeira gravação, o arquivo anterior é copiado para `library.ini.backup`, se esse backup ainda não existir.

Não abra a versão antiga depois de cadastrar capas/favoritos: ela não conhece os novos campos e pode removê-los ao salvar. Para voltar à v1, encerre o monitor e restaure manualmente seu backup.

## Primeiro build pelo GitHub, sem instalar compilador

Se ainda não tem repositório:

1. Crie um repositório vazio na sua conta GitHub.
2. Extraia o ZIP e abra o CMD **dentro da pasta FeatherPad**, onde está `CMakeLists.txt`.
3. Com o Git instalado, execute, substituindo a URL:

```cmd
git init
git add .
git commit -m "FeatherPad 2"
git branch -M main
git remote add origin https://github.com/SEU-USUARIO/featherpad.git
git push -u origin main
```

4. Abra **Actions > Build Windows**. Depois de ficar verde, baixe o artefato **FeatherPad-Windows-x64**. Você precisa estar conectado ao GitHub para baixar artefatos.
5. Extraia o EXE para uma pasta fixa, por exemplo `C:\Apps\FeatherPad`, e abra.

O mesmo workflow compila no Windows, testa a migração dos dados, a importação de imagens, a interface e a configuração de botões, e publica o executável. O relatório fica no artefato **FeatherPad-test-report**. Se algo falhar, consulte esse relatório e o log da execução.

## Biblioteca e capas

- Clique **Adicionar jogo** ou pressione **A**.
- Informe o nome e clique **Procurar** para escolher um `.exe`, `.lnk` ou `.url`. Também pode informar uma URI como `steam://rungameid/292030`.
- Preencha argumentos e pasta de trabalho somente se o jogo precisar. O caminho do executável não deve ter aspas externas; os argumentos podem ter aspas.
- Use **Escolher capa** e selecione um PNG, JPG ou BMP local. Recomendado: capa vertical, aproximadamente 600 × 900 pixels. Limite de entrada: 2048 × 2048 pixels e 8 MB. PNG transparente recebe fundo escuro/preto na conversão.
- Opcionalmente, escreva uma descrição curta e marque favorito.
- Clique **Salvar jogo**.

A imagem é convertida e copiada para o perfil, em até **300 × 420 pixels**, mantendo a proporção. Você pode mover ou apagar a imagem original depois de salvar. Capas não são baixadas automaticamente. Se faltar uma imagem, o jogo continua funcionando e recebe uma arte com seu nome. Capas são recortadas no centro para preencher os cartões.

Selecione um cartão para destacar o jogo. Use **Jogar agora**, Enter ou o botão configurado de confirmar. Duplo clique na capa também inicia o jogo. Para alterar, use **Editar** ou **E**. Para excluir, abra Editar e escolha **Excluir da lista**: os arquivos do jogo são preservados.

Clientes como Steam continuam necessários quando exigidos pelo jogo. FeatherPad abre o executável/atalho/URI; não substitui autenticação da loja nem detecta jogos automaticamente.

## Perfil e favoritos

Clique no perfil no canto inferior esquerdo ou pressione **P** para trocar nome e imagem. Esse perfil é local à sua conta do Windows, sem login ou cadastro online. Não é um sistema de múltiplos perfis.

Use a estrela no painel do jogo, a tecla **F** ou o botão de favoritar no controle. A seção **Favoritos** mostra somente os jogos marcados. Todos os dados ficam salvos entre execuções.

## Configurar o controle — passo a passo

1. Abra **Configurações** na barra lateral, ou pressione **S**.
2. Na ação desejada, clique no botão **Alterar**.
3. Um pop-up mostra exatamente o que está sendo configurado: abrir o launcher, confirmar/jogar, voltar/ocultar ou favoritar.
4. **Solte todos os botões primeiro.** O programa só aceita uma nova pressão depois de receber uma entrada com os botões soltos daquele controle.
5. Pressione o botão desejado. Se estiver atribuído a outra ação, o pop-up informa o conflito e espera outro botão.
6. Confira o número detectado e clique **Salvar botão** ou pressione Enter. A alteração só é aplicada nesse momento.
7. **Cancelar**, o X do pop-up ou Esc preservam o botão anterior.

Depois que um candidato foi identificado, os botões atuais de confirmar e voltar também podem salvar/cancelar. Durante a espera inicial, use mouse ou Esc para cancelar, pois o programa precisa poder capturar inclusive o seu botão de voltar.

Cada ação usa um botão diferente; combinações não são suportadas. Para trocar dois botões entre si, atribua temporariamente um botão livre a uma das ações.

Mapeamento inicial comum em controles Sony (os números reais podem variar):

| Ação | Botão HID inicial | Referência comum |
|---|---:|---|
| Abrir/ocultar launcher | 13 | PS |
| Confirmar/jogar | 2 | X |
| Voltar/ocultar | 3 | Círculo |
| Favoritar | 4 | Triângulo |

Configurações personalizadas da v1 são preservadas. Se o novo favorito colidir com uma delas, é escolhido um número livre; confira em Configurações.

**Testar os botões do controle** abre uma tela de diagnóstico: ela mostra os números recebidos e **não altera nenhum mapeamento**. Nessa tela, use Concluir ou Esc para sair.

## Navegação

Na biblioteca:

- Direcional/analógico para esquerda e direita: passar pelos jogos.
- Para cima/baixo: alcançar as ações da interface; a borda verde mostra o foco. Ao chegar a um cartão, esquerda/direita voltam a navegar pela coleção.
- Confirmar: executar a ação em foco ou iniciar o jogo selecionado.
- Favoritar: alternar a estrela do jogo selecionado.
- Voltar: ocultar a biblioteca.

Nas janelas de configurações, cadastro ou perfil, o direcional alterna entre os campos/botões e confirmar aciona o botão em foco. Digitação de texto e seleção de arquivos usam teclado/mouse. Na captura, o foco fica no pop-up para evitar abrir jogos por acidente.

Teclado: setas para navegar, Tab/Shift+Tab para percorrer ações, Enter para confirmar, F para favorito, S para configurações, A para adicionar, E para editar, P para perfil e Esc para voltar/ocultar.

## Bluetooth, USB e segundo plano

Primeiro pareie o controle no Windows ou conecte por USB. FeatherPad recebe joystick/gamepad HID pelo Raw Input, inclusive quando está oculto. Não abre uma segunda conexão Bluetooth e não modifica drivers.

**O monitor precisa estar rodando para detectar o controle.** Ligar o controle não inicia um aplicativo totalmente encerrado. Em Configurações, ative **Iniciar com o Windows** para que o monitor fique pronto após o login.

**Mostrar a biblioteca ao conectar o controle** é opcional; por padrão, a janela só aparece ao usar o botão de abertura. O monitor continua sem animações nem polling periódico de Bluetooth.

X, Esc e o botão de voltar ocultam a janela. Ao iniciar um jogo, ela também é ocultada e o cache de capas é liberado. Para encerrar de verdade: clique com o botão direito no ícone da bandeja e escolha **Encerrar monitor**.

Sem a compatibilidade ativada, controles apresentados exclusivamente como XInput ainda usam o backend anterior limitado. Com a compatibilidade ativada, a leitura SDL também atende controles XInput reconhecidos. DualShock 4 e DualSense dependem do descriptor HID exposto pelo Windows. Steam Input, DS4Windows e overlays podem ocultar dispositivos ou responder ao mesmo botão. Para diagnosticar, teste primeiro sem remapeadores. O programa não bloqueia o botão PS para outros aplicativos. O Windows também pode limitar a tomada de foco; nesse caso, use o ícone da bandeja.

## Leveza e limites de memória

- Um processo e uma thread de interface, aguardando eventos do Windows.
- Interface desenhada com GDI e fontes do sistema, sem motor de navegador ou GPU de jogo.
- Imagens lidas com Windows Imaging Component, convertido para miniaturas locais.
- Cache máximo de seis imagens de até 300 × 420 pixels: cerca de 2,9 MiB de pixels, além das estruturas do programa.
- Cache liberado ao ocultar/minimizar a biblioteca; nenhum timer de animação ou vídeo.
- Buffer de pintura temporário, liberado ao terminar de desenhar.

**A meta continua sendo 20–50 MB de RAM, mas não há garantia nem medição no seu PC nesta entrega.** Codecs de imagem, resolução da janela, escala do Windows e bibliotecas do sistema afetam o pico e o working set. A importação pode consumir mais transitoriamente que a janela oculta. Para economizar, use capas pequenas e evite maximizar em monitores de resolução muito alta.

Confira FeatherPad.exe no Gerenciador de Tarefas após um minuto: janela aberta, janela oculta, controle desconectado e jogo rodando. Para comparar corretamente, registre a mesma métrica (working set ou bytes privados) em cada cenário. A versão 2 acrescenta imagens e, portanto, precisa ser medida novamente em relação à v1.

## Dados, backup e desinstalação

Dados: `%LOCALAPPDATA%\FeatherPad\library.ini` e a subpasta `art` com as miniaturas. Faça backup da pasta inteira com o monitor encerrado. O EXE pode ficar em qualquer pasta fixa.

Não há telemetria, download de capas, coleta de perfil ou login. Limite de 2000 jogos; descrições têm até 160 caracteres e nome de perfil até 40. Outros campos têm até 8000, sem quebras de linha.

Ao remover uma capa ou um jogo, a miniatura importada permanece em `art` para evitar apagar uma imagem ainda referenciada. Você pode limpar arquivos órfãos manualmente com o monitor encerrado, após fazer backup.

Para desinstalar: desative Iniciar com o Windows, encerre o monitor e apague a pasta do EXE. Se quiser eliminar o cadastro e as capas, apague também a pasta de dados. Jogos originais são preservados.

## Compilação local opcional

Com Visual Studio Build Tools, componente **Desenvolvimento para desktop com C++**, CMake 3.24+ e SDK do Windows, abra **x64 Native Tools Command Prompt for VS** nesta pasta e execute `build.cmd`. Resultado: `build\Release\FeatherPad.exe`, acompanhado de `SDL3.dll`.

Com CMake:

```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

O runtime C++ e o cliente ViGEm são vinculados estaticamente. Mantenha a SDL3.dll distribuída no pacote ao lado do EXE. Quem usa o programa não precisa instalar ferramentas de desenvolvimento; a conversão virtual exige o driver descrito acima. O executável não é assinado digitalmente.

## Verificação desta entrega

A versão anterior foi testada pelo usuário e funcionou no seu PC. Esta atualização foi compilada para Windows x64 por compilação cruzada com GCC/MinGW, com avisos tratados como erros. Isso verifica sintaxe, tipos, recursos e linkedição; não substitui executar no Windows.

Os testes nativos incluídos rodam pelo GitHub Actions no Windows: migração, categorias, estatísticas, perfis de controle, calibração, imagens, tradução e quatro entradas SDL virtuais. O teste automatizado não substitui a verificação de quatro controles físicos, Bluetooth, bateria e slots do driver no seu computador.

Depois do build, confira:

1. Seus jogos antigos continuam presentes.
2. Uma capa aparece e permanece após reiniciar o monitor, mesmo movendo a imagem original.
3. Favoritos, nome e imagem de perfil persistem.
4. Os quatro pop-ups de botões mostram a ação correta; cancelar não altera o cadastro e botões duplicados são rejeitados.
5. O controle navega entre jogos e ações, abre o jogo e oculta o launcher.
6. O monitor abre pelo botão com a janela oculta, por USB e Bluetooth.
7. Inicialização com Windows e reconexão continuam funcionando.
8. RAM e CPU permanecem adequadas no seu computador.

## Organização do código

- `src/main.cpp`: inicialização, instância única e loop de mensagens.
- `src/ui.cpp`: biblioteca, formulários, navegação e captura explícita.
- `src/input.cpp`: eventos HID, botões e eixos.
- `src/storage.cpp`: cadastro, migração, inicialização com Windows e execução de jogos.
- `src/art.cpp`: leitura/conversão de capas e cache limitado.
- `src/controller_state.cpp`: modelo independente de controle e conversões Xbox/PlayStation.
- `src/gamepad_bridge.cpp`: entrada SDL, ciclo de vida dos controles virtuais e proteção contra realimentação.
- `tests/controller_state_tests.cpp`: regressões da tradução de entradas.
- `tests/smoke.cpp`: testes nativos isolados, sem alterar seu cadastro real.
- `.github/workflows/build.yml`: compilação e verificação no Windows.

Referências técnicas: [Raw Input](https://learn.microsoft.com/en-us/windows/win32/inputdev/raw-input), [HID](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/interpreting-hid-reports) e [WIC Bitmap Scaler](https://learn.microsoft.com/en-us/windows/win32/api/wincodec/nn-wincodec-iwicbitmapscaler).

