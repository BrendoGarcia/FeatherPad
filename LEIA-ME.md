# FeatherPad 2 — sua biblioteca de jogos

Launcher nativo para Windows x64, escrito em C++17 e Win32. Sem navegador, Electron, Java, Python, conta ou conexão com a internet em tempo de execução.

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
6. Na execução verde, baixe o artefato **FeatherPad-Windows-x64**, extraia e substitua o seu executável anterior.
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

Controles apresentados exclusivamente como XInput ainda não têm backend dedicado. DualShock 4 e DualSense dependem do descriptor HID exposto pelo Windows. Steam Input, DS4Windows e overlays podem ocultar dispositivos ou responder ao mesmo botão. Para diagnosticar, teste primeiro sem remapeadores. O programa não bloqueia o botão PS para outros aplicativos. O Windows também pode limitar a tomada de foco; nesse caso, use o ícone da bandeja.

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

Com Visual Studio Build Tools, componente **Desenvolvimento para desktop com C++** e SDK do Windows, abra **x64 Native Tools Command Prompt for VS** nesta pasta e execute `build.cmd`. Resultado: `dist\FeatherPad.exe`.

Com CMake:

```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

O runtime C++ é vinculado estaticamente; quem só usa o EXE não precisa instalar ferramentas de desenvolvimento. O executável não é assinado digitalmente.

## Verificação desta entrega

A versão anterior foi testada pelo usuário e funcionou no seu PC. A versão 2 foi compilada para Windows x64 por compilação cruzada com GCC/MinGW, com avisos tratados como erros. Isso verifica sintaxe, tipos, recursos e linkedição; não substitui executar no Windows.

Os testes nativos incluídos rodam pelo GitHub Actions no Windows: migração da v1, Unicode/aspas, favoritos, backup, importação e cache de imagens, abertura das janelas e captura de botões simulada. Não houve teste físico de Bluetooth/USB nem medição de RAM nesta entrega. Não afirmamos que os testes Windows passaram antes de executar o workflow no seu repositório.

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
- `tests/smoke.cpp`: testes nativos isolados, sem alterar seu cadastro real.
- `.github/workflows/build.yml`: compilação e verificação no Windows.

Referências técnicas: [Raw Input](https://learn.microsoft.com/en-us/windows/win32/inputdev/raw-input), [HID](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/interpreting-hid-reports) e [WIC Bitmap Scaler](https://learn.microsoft.com/en-us/windows/win32/api/wincodec/nn-wincodec-iwicbitmapscaler).
