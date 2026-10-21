<div align="center">

# 🎮 FeatherPad

### Sua biblioteca. Seu controle. Direto ao jogo.

Um launcher nativo para Windows que reúne seus jogos, permite navegar pelo controle e oferece conversão de entradas para jogos com diferentes formatos de gamepad.

[![Build Windows](https://github.com/BrendoGarcia/FeatherPad/actions/workflows/build.yml/badge.svg)](https://github.com/BrendoGarcia/FeatherPad/actions/workflows/build.yml)
![Windows](https://img.shields.io/badge/Windows-10%20%2F%2011%20x64-0078D4)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C)
[![Licença MIT](https://img.shields.io/badge/Licen%C3%A7a-MIT-green.svg)](LICENSE)

**Interface nativa · Biblioteca com capas · Favoritos · Controle USB/Bluetooth · Build pelo GitHub**

[📦 Downloads](https://github.com/BrendoGarcia/FeatherPad/releases) · [⚙️ Compilar pelo GitHub](#️-build-pelo-github-sem-instalar-compilador) · [📖 Guia operacional](LEIA-ME.md) · [🐛 Relatar problema](https://github.com/BrendoGarcia/FeatherPad/issues)

</div>

---

## ✨ O que é o FeatherPad?

O FeatherPad foi criado para transformar o PC em uma biblioteca de jogos acessível pelo controle, com foco em simplicidade e baixo consumo de recursos. Conecte seu controle, abra a biblioteca pelo botão configurado, escolha um jogo e jogue.

A interface é escrita em **C++17 e Win32**, com desenho nativo em GDI. Não usa navegador, Electron, Java ou Python para funcionar. A biblioteca fica no seu computador, sem conta online ou serviço de assinatura.

Além de organizar e abrir jogos, o launcher pode ler um controle compatível e apresentá-lo ao jogo como **Xbox 360**, **PlayStation 4** ou deixar o dispositivo no **modo nativo**. A preferência global atende à biblioteca inteira; exceções podem ser salvas por jogo.

> **Leveza é uma prioridade de projeto.** A meta original é 20–50 MB de RAM, mas isso não é uma medição nem um limite garantido. Capas, resolução da janela e compatibilidade de controles afetam o consumo. Veja [desempenho](#-desempenho-e-consumo-de-recursos).

## 🧭 Conteúdo

- [Recursos](#-recursos)
- [Instalação e primeiros passos](#-instalação-e-primeiros-passos)
- [Biblioteca, capas e favoritos](#-biblioteca-capas-e-favoritos)
- [Navegação pelo controle e teclado](#-navegação-pelo-controle-e-teclado)
- [Configuração dos botões](#-configuração-dos-botões)
- [Compatibilidade de controles nos jogos](#-compatibilidade-de-controles-nos-jogos)
- [Segundo plano e inicialização com Windows](#-segundo-plano-e-inicialização-com-windows)
- [Desempenho e consumo de recursos](#-desempenho-e-consumo-de-recursos)
- [Dados, atualização e backup](#-dados-atualização-e-backup)
- [Build pelo GitHub](#️-build-pelo-github-sem-instalar-compilador)
- [Compilação local e testes](#-compilação-local-e-testes)
- [Solução de problemas](#-solução-de-problemas)
- [Código, contribuições e licença](#-código-contribuições-e-licença)

## 🚀 Recursos

| Recurso | O que você pode fazer |
|---|---|
| **Pesquisa por nome** | Encontrar jogos por partes do título, ignorando maiúsculas e acentos, inclusive nos Favoritos. |
| **Teclado virtual** | Pesquisar e editar campos de texto pelo controle, com aplicação/cancelamento e cursor. |
| **Tela cheia / TV** | Usar a biblioteca sem bordas, com escala dos elementos e restauração da janela pelo F11. |
| **Atualização pelo aplicativo** | Consultar Releases, baixar arquivos alterados com SHA-256 e aplicar pelo Updater separado. |
| **Biblioteca visual** | Navegar por cartões com capas, carrossel e destaque do jogo selecionado. |
| **Cadastro de jogos** | Adicionar executáveis, atalhos, URLs e URIs de lojas, com argumentos e pasta de trabalho opcionais. |
| **Favoritos** | Marcar seus jogos preferidos e acessar uma coleção filtrada. |
| **Perfil local** | Personalizar o nome e a imagem exibidos na biblioteca. |
| **Navegação pelo controle** | Percorrer jogos, alcançar ações e iniciar partidas sem usar o mouse na biblioteca. |
| **Foco visível** | Identificar a ação em foco por uma borda branca de alto contraste com separação escura. |
| **Botões configuráveis** | Escolher abrir/ocultar, confirmar, voltar e favoritar em pop-ups com confirmação e detecção de conflitos. |
| **Diagnóstico de entrada** | Consultar os botões recebidos sem alterar o mapeamento. |
| **Compatibilidade opcional** | Usar entrada SDL e saída nativa, Xbox 360 virtual ou DualShock 4 virtual. |
| **Preferência por jogo** | Manter o padrão global ou salvar um formato específico para um título. |
| **Uso em segundo plano** | Ocultar a biblioteca, abrir pelo controle e manter o monitor na bandeja do Windows. |
| **Inicialização automática** | Iniciar o monitor junto com o login no Windows e, opcionalmente, mostrar a biblioteca ao conectar o controle. |
| **Build na nuvem** | Gerar o pacote Windows pelo GitHub Actions sem instalar compilador no seu PC. |

A janela preserva seu estado maximizado ao abrir configurações e ao ocultar/reabrir o launcher. Fechar a biblioteca pelo **X** mantém o monitor ativo.

## 📦 Instalação e primeiros passos

### Requisitos

- **Windows 10 ou Windows 11, 64 bits**.
- Um controle compatível, pareado no Windows por Bluetooth ou conectado por USB.
- Jogos já instalados ou acessíveis pelo cliente da loja correspondente.
- Para saídas virtuais: **ViGEmBus 1.22.0**, instalado uma vez com permissão de administrador.

Não é necessário instalar ferramentas de desenvolvimento para usar um pacote compilado. A conversão não exige manter Steam ou DS4Windows aberto; o cliente da loja ainda será necessário se o próprio jogo depender dele.

### Baixar e abrir

1. Acesse [Releases](https://github.com/BrendoGarcia/FeatherPad/releases) e procure um pacote Windows compilado. Se ainda não houver uma release, use o [GitHub Actions](#️-build-pelo-github-sem-instalar-compilador).
2. Extraia **todo o conteúdo** do pacote para uma pasta fixa, por exemplo `C:\Apps\FeatherPad`.
3. Mantenha `SDL3.dll` e `Updater.exe` ao lado de `FeatherPad.exe` e preserve a pasta `licenses` e as instruções distribuídas.
4. Execute `FeatherPad.exe`.
5. Clique em **Adicionar jogo**, informe o nome e o caminho e salve.
6. Conecte o controle, confira os botões em **Configurações** e selecione **Jogar agora**.

> Os arquivos **Source code (zip/tar.gz)** que o GitHub oferece em uma release são código-fonte. Para executar sem compilar, baixe o pacote Windows que contém o EXE e a DLL.

A compatibilidade de controles vem **desativada inicialmente**. Para ativá-la, siga o [guia abaixo](#-compatibilidade-de-controles-nos-jogos).

## 🖼️ Biblioteca, capas e favoritos

### Novidades de uso na versão 2.2

Use **Pesquisar jogos** na barra lateral (ou **F3/Q**) para abrir o teclado virtual e filtrar nomes. **Aplicar** confirma e **Cancelar/Voltar** preserva a busca anterior. **Limpar pesquisa** remove o filtro. A busca combina com Favoritos.

Em campos editáveis de cadastro/perfil, **Confirmar** no controle abre o teclado virtual. Ele tem letras, números, acentos, símbolos, maiúsculas, apagar e movimento do cursor. Aplicar preenche o campo; salvar o formulário continua sendo uma ação separada.

**Tela cheia / TV**, também disponível nas configurações e por **F11**, ocupa o monitor sem bordas, escala os elementos e restaura a janela anterior ao sair.

### Adicionar ou editar um jogo

Use **Adicionar jogo** ou a tecla **A**. Você pode cadastrar:

| Destino | Exemplo |
|---|---|
| Executável | `D:\Jogos\MeuJogo\MeuJogo.exe` |
| Atalho do Windows | Arquivo `.lnk` |
| Atalho de internet | Arquivo `.url` |
| URI de uma loja | `steam://rungameid/292030` |

Informe argumentos e pasta de trabalho somente quando o jogo precisar. Não coloque aspas externas no campo do executável; argumentos podem conter aspas. O cadastro também aceita uma descrição curta, favorito e preferência de controle.

Para alterar um jogo, selecione-o e use **Editar** ou **E**. **Excluir da lista** remove apenas o cadastro: os arquivos do jogo permanecem no computador.

O FeatherPad não procura instalações automaticamente, não baixa jogos e não substitui a autenticação das lojas.

### Capas

Use **Escolher capa** e selecione uma imagem local PNG, JPG ou BMP. Uma capa vertical, aproximadamente **600 × 900 pixels**, funciona bem.

- Entrada máxima: **2048 × 2048 pixels e 8 MB**.
- A imagem é copiada e convertida para uma miniatura PNG de até **300 × 420 pixels**, mantendo a proporção.
- Os cartões usam recorte central para preencher a área disponível.
- Depois de salvar, você pode mover a imagem original.
- Sem capa, o launcher exibe uma composição com o nome do jogo.

As capas são escolhidas pelo usuário; não há download automático de artes.

### Favoritos e perfil

Use a estrela, a tecla **F** ou o botão configurado para favoritar. A seção **Favoritos** reúne os jogos marcados.

Clique no perfil ou pressione **P** para mudar nome e imagem. É um perfil local por conta do Windows, sem login online ou alternância entre múltiplos perfis.

## 🕹️ Navegação pelo controle e teclado

### No controle

| Entrada | Na biblioteca |
|---|---|
| Direcional/analógico para esquerda ou direita | Percorrer os jogos. |
| Direcional/analógico para cima ou baixo | Alcançar ações da interface. |
| Confirmar | Acionar o botão em foco ou iniciar o cartão em foco. |
| Favoritar | Marcar/desmarcar o jogo selecionado. |
| Voltar | Ocultar a biblioteca. |
| Abrir launcher | Mostrar/ocultar a biblioteca. |

A **borda branca com separação escura** indica onde o controle está atuando. O destaque do jogo selecionado e o foco da ação são estados diferentes.

Nas janelas de configurações, perfil e cadastro, o direcional percorre os controles do formulário e confirmar aciona botões. Digitar nomes e selecionar arquivos ainda exige teclado/mouse.

### No teclado

| Tecla | Ação |
|---|---|
| Setas | Navegar. |
| Tab / Shift + Tab | Percorrer ações. |
| Enter / Espaço | Confirmar a ação em foco. |
| A | Adicionar jogo. |
| E | Editar jogo selecionado. |
| F | Alternar favorito. |
| S | Abrir configurações. |
| P | Editar perfil. |
| Esc | Voltar/ocultar ou cancelar o diálogo atual. |

## 🎛️ Configuração dos botões

Em **Configurações**, cada ação tem seu próprio botão **Alterar**:

1. Abra a captura da ação desejada.
2. Leia a ação indicada no pop-up e **solte todos os botões**.
3. Pressione o botão que deseja usar.
4. Confira o número identificado. Se houver conflito com outra ação, escolha outro botão.
5. Selecione **Salvar botão** para aplicar.

**Cancelar**, **Esc** ou fechar o pop-up preserva a configuração anterior. Cada ação precisa de um botão diferente; combinações de botões não são suportadas. Para trocar dois botões entre si, use temporariamente um terceiro botão livre.

| Ação | Padrão inicial | Referência comum |
|---|---:|---|
| Abrir/ocultar | 13 | PS / Guide |
| Confirmar/jogar | 2 | X do PlayStation / A do Xbox |
| Voltar/ocultar | 3 | Círculo / B |
| Favoritar | 4 | Triângulo / Y |

Com compatibilidade ativada, os números representam posições padronizadas pela SDL. No modo HID original, os números podem variar conforme o controle. Confira seus atalhos ao alternar o backend, especialmente em réplicas.

**Testar os botões do controle** abre um diagnóstico separado: ele mostra as entradas e não modifica os atalhos. Durante a espera inicial da captura, use mouse ou Esc para cancelar, pois o botão de voltar também pode ser o botão que você quer cadastrar.

## 🔄 Compatibilidade de controles nos jogos

### Como funciona

A camada opcional usa **SDL 3.4.18** para ler controles reconhecidos e **ViGEmBus** para criar uma saída virtual. Ela traduz botões, direcional, dois analógicos e gatilhos para o formato escolhido.

| Modo | O que o jogo recebe | Exige ViGEmBus? |
|---|---|---|
| **Nativo — sem conversão** | O controle físico, conforme o suporte do jogo. | Não |
| **Xbox 360 virtual** | Um controle Xbox 360 virtual, via XInput. | Sim |
| **PlayStation 4 virtual** | Um DualShock 4 virtual. | Sim |

A entrada aceita modelos de Xbox, PlayStation, Switch e outros fabricantes reconhecidos pela SDL, por USB/Bluetooth conforme o dispositivo e o driver. **O controle físico não precisa ser da mesma marca da saída escolhida.**

### Ativar uma vez

1. Abra **Configurações > Compatibilidade dos jogos**.
2. Para saída virtual, use **Driver necessário** e instale o [ViGEmBus 1.22.0 oficial](https://github.com/nefarius/ViGEmBus/releases/tag/v1.22.0). Reinicie o Windows se o instalador pedir.
3. Ative **Ativar compatibilidade de controles**.
4. Escolha o formato global: Xbox 360, PlayStation 4 ou Nativo.
5. Deixe **Controle de origem: Automático** ou selecione um dispositivo reconhecido.
6. Confira o status. Se instalou o driver com o launcher aberto, use **Verificar novamente**.
7. Inicie um jogo. Para conferir o dispositivo virtual no Windows, abra `joy.cpl`.

O padrão inicial de saída é Xbox 360. A preferência fica salva e os jogos usam **Usar padrão global**, evitando repetir a configuração para cada título.

### Exceções por jogo

Se um título precisar de outro formato, abra **Editar > Controle para este jogo** e escolha **Usar padrão global**, **Nativo**, **Xbox 360 virtual** ou **PlayStation 4 virtual**.

O formato é aplicado antes de iniciar o jogo. Ele permanece ativo até o próximo jogo ou uma nova configuração global; o launcher não acompanha o encerramento de processos de clientes externos.

**Não existe detecção universal do formato aceito por todo jogo.** A seleção global atende aos jogos compatíveis com aquele formato; as exceções ficam salvas individualmente. O launcher não cria suporte a gamepad em jogos que não o possuem nem garante suporte a todo controle existente.

### Controle não reconhecido

Use **Importar mapeamentos SDL...** para importar um `gamecontrollerdb.txt` compatível. O arquivo, de até 4 MB, é copiado para a pasta de dados e aplicado na próxima inicialização da camada de controles. Não executa código.

Um mapeamento descreve o controle físico e pode ser reutilizado em toda a biblioteca. Um joystick sem perfil SDL pode precisar desse cadastro inicial para aparecer como gamepad.

### Evitar entrada dupla

Alguns jogos enxergam o controle físico e o virtual ao mesmo tempo, causando comandos duplicados. Para esses casos, **Evitar entrada dupla** abre o [guia oficial do HidHide](https://docs.nefarius.at/projects/HidHide/Simple-Setup-Guide/), um filtro opcional separado.

- Permita `FeatherPad.exe` na lista de aplicativos do HidHide.
- Oculte apenas o controle **físico** dos outros aplicativos.
- Mantenha o controle **virtual** visível.
- Para jogar em modo Nativo, desative a ocultação do físico quando necessário.

O FeatherPad não configura o HidHide automaticamente. No primeiro teste, feche outros conversores, como Steam Input e DS4Windows, para evitar sobreposição.

### Comportamento e limites

- **Um controle/jogador por vez**. Controles idênticos podem compartilhar o GUID; nesse caso, a seleção de origem usa o primeiro disponível.
- O botão **PS/Guide** fica reservado ao launcher e não é enviado à saída virtual.
- Ao mostrar a biblioteca, a saída virtual fica neutra. Depois de iniciar um jogo, o encaminhamento aguarda os comandos voltarem ao repouso.
- Desconectar o físico remove a saída virtual; reconectar recria o dispositivo.
- O monitor precisa continuar aberto em segundo plano durante o jogo.
- Esta versão não encaminha vibração, giroscópio, áudio, LEDs, gatilhos adaptativos ou touchpad como ponteiro. A saída PlayStation é DualShock 4, não emulação completa de DualSense.
- Políticas do Windows, jogos e anti-cheat podem restringir dispositivos virtuais. Não há injeção de DLL no jogo ou bypass de anti-cheat.

> **Dependência externa:** ViGEmBus e ViGEmClient são projetos aposentados. A integração é opcional e utiliza a versão oficial 1.22.0 do driver, que removeu o atualizador legado. Consulte o [aviso de fim de vida](https://docs.nefarius.at/projects/ViGEm/End-of-Life/) e evite antigos atualizadores.

## 🔄 Atualização dentro do launcher

Em **Configurações > Buscar atualizações**, o FeatherPad consulta a última Release estável pública, compara versões e oferece o download. Apenas arquivos com tamanho/hash diferentes são baixados; cada arquivo alterado é baixado inteiro.

Depois de validar os digests SHA-256 fornecidos pelo GitHub, o launcher solicita confirmação para reiniciar. Uma cópia do `Updater.exe` instalado aguarda o encerramento, preserva os arquivos anteriores para recuperação, aplica os arquivos permitidos e reabre o programa. A biblioteca do usuário não é substituída. Evite atualizar durante uma partida, pois o monitor será encerrado.

**Para migrar de uma versão anterior a 2.2, baixe o pacote completo uma vez**, incluindo `Updater.exe`. Depois disso, as versões futuras podem ser instaladas pelo botão, desde que a Release contenha os assets individuais produzidos pelo workflow **Release Windows**. A versão 2.1 não consegue atualizar a si mesma por esse mecanismo.

As consultas são manuais. HTTPS valida a origem e SHA-256 verifica a integridade; **não há assinatura digital independente do publicador**. O mecanismo confia na conta/release oficial do GitHub. Release privada/incompleta, digest ausente, falha de rede ou permissões insuficientes geram aviso. Não há token embutido nem elevação automática.

Recuperações e logs ficam em `%LOCALAPPDATA%\FeatherPad\updates`. Consulte o [guia de atualização e publicação](LEIA-ME.md) para rollback e os sete assets necessários. Publicar código na branch não disponibiliza, por si só, uma atualização: o mantenedor precisa publicar uma Release compilada.

## 🌙 Segundo plano e inicialização com Windows

Pareie o controle pelo próprio Windows antes de usar Bluetooth. O launcher lê os dispositivos apresentados pelo sistema; não realiza o pareamento por conta própria.

Em **Configurações**, você pode ativar:

- **Iniciar com o Windows:** prepara o monitor após o login, com a biblioteca oculta.
- **Mostrar a biblioteca ao conectar o controle:** abre a interface na conexão, se preferir esse comportamento.

**O monitor precisa estar rodando para detectar conexões e botões.** Um controle não inicia o processo se ele estiver completamente encerrado.

O X da janela e o botão de voltar ocultam a biblioteca. Ao iniciar um jogo, ela também é ocultada. Para abrir novamente, use o botão configurado ou o ícone na bandeja. Para sair de verdade, clique com o botão direito no ícone e escolha **Encerrar monitor**.

O monitor usa instância única. Se mover o executável após configurar a inicialização automática, desative e reative a opção para atualizar o caminho.

## 🪶 Desempenho e consumo de recursos

A arquitetura evita um motor de navegador e mantém a interface simples:

- UI Win32/GDI com fontes do sistema, sem animações ou vídeos.
- Miniaturas locais em vez de carregar capas grandes durante a navegação.
- Cache limitado a seis miniaturas de até 300 × 420 pixels: aproximadamente **2,9 MiB de pixels**, além das demais estruturas.
- Cache liberado ao ocultar/minimizar e buffer de pintura temporário.
- Backend Raw Input orientado a eventos quando a compatibilidade está desativada.
- SDL carregada sob demanda quando a compatibilidade está ativada.

No modo de compatibilidade há atualização periódica de entrada, aproximadamente a cada **8–16 ms** com controle e **250 ms** sem controle. Esse modo acrescenta consumo de CPU/RAM.

O consumo total depende da resolução, escala do Windows, bibliotecas carregadas e operações como importação de imagens. Para avaliar no seu PC, compare a mesma métrica no Gerenciador de Tarefas com a janela aberta, oculta e durante um jogo. **Não há benchmark publicado que garanta a meta de 20–50 MB.**

## 💾 Dados, atualização e backup

| Local | Conteúdo |
|---|---|
| `%LOCALAPPDATA%\FeatherPad\library.ini` | Jogos, favoritos, perfil, botões e preferências. |
| `%LOCALAPPDATA%\FeatherPad\art\` | Capas e imagem de perfil importadas. |
| `%LOCALAPPDATA%\FeatherPad\gamecontrollerdb.txt` | Mapeamentos SDL importados, quando houver. |
| `%LOCALAPPDATA%\FeatherPad\library.ini.backup` | Cópia anterior criada antes de uma gravação, se ainda não existir. |

Para um backup completo, **encerre o monitor e copie toda a pasta** `%LOCALAPPDATA%\FeatherPad`. O arquivo `.backup` não é um histórico contínuo de versões.

### Atualizar

1. Encerre o monitor pela bandeja.
2. Faça backup da pasta de dados.
3. Substitua os arquivos do pacote, incluindo `SDL3.dll` e licenças.
4. Abra a nova versão e confira sua biblioteca.

Cadastros anteriores são migrados e recebem valores padrão nos campos novos. Evite executar uma versão antiga depois de atualizar: ao salvar, ela pode descartar campos que não conhece.

A biblioteca suporta até **2000 jogos**, descrições de até **160 caracteres** e nome de perfil de até **40 caracteres**. Miniaturas removidas do cadastro podem permanecer em `art` para evitar apagar imagens ainda referenciadas; limpe arquivos órfãos apenas após backup.

### Privacidade e remoção

Não há login, telemetria, coleta de perfil ou download automático de capas. O aplicativo não baixa bibliotecas em tempo de execução. Links de ajuda abrem páginas externas por ação do usuário; jogos e clientes de lojas têm suas próprias necessidades de rede.

Para remover o launcher, desative **Iniciar com o Windows**, encerre o monitor e apague a pasta do programa. Se desejar apagar seus dados, remova também `%LOCALAPPDATA%\FeatherPad`. Os jogos instalados não são excluídos. Drivers opcionais são componentes separados e possuem sua própria desinstalação.

## ⚙️ Build pelo GitHub sem instalar compilador

O workflow [**Build Windows**](https://github.com/BrendoGarcia/FeatherPad/actions/workflows/build.yml) compila e executa testes em um runner Windows do GitHub.

### Baixar uma compilação existente

1. Abra **Actions > Build Windows**.
2. Selecione uma execução concluída com sucesso.
3. Em **Artifacts**, baixe **FeatherPad-Windows-x64**. O GitHub exige login para baixar artefatos.
4. Extraia o pacote inteiro e execute `FeatherPad.exe`.

O pacote inclui EXE, `Updater.exe`, `SDL3.dll`, licenças das dependências e `LEIA-ME.md`. Os logs ficam em **FeatherPad-test-report**. Artefatos têm retenção limitada pelo GitHub; releases são o local indicado para disponibilizar versões aos usuários.

### Gerar sua própria compilação

1. Faça um **fork** do repositório.
2. Habilite GitHub Actions no fork, caso solicitado.
3. Abra **Actions > Build Windows > Run workflow**.
4. Aguarde a execução verde e baixe o artefato.

Não é necessário instalar Visual Studio ou CMake no seu computador. O workflow também roda automaticamente em pushes que alteram código, testes, configuração de build, `LEIA-ME.md` ou o próprio workflow. Mudanças apenas no README não disparam o build automaticamente.

<details>
<summary><strong>📋 Para mantenedores: preparar uma release</strong></summary>

1. Escolha o commit que será distribuído e confirme o build/testes no Actions.
2. Teste o pacote extraído em Windows com um controle e jogos reais.
3. Baixe **FeatherPad-Windows-x64** daquela execução.
4. Crie uma tag/release correspondente ao commit e anexe o ZIP compilado completo.
5. Descreva mudanças, requisitos e dependências opcionais nas notas.
6. Confira o download: `FeatherPad.exe`, `SDL3.dll`, instruções e licenças devem estar presentes.

O executável não é assinado digitalmente. O projeto não distribui silenciosamente instaladores de drivers; mantenha a referência ao download oficial do ViGEmBus.

</details>

## 🛠️ Compilação local e testes

### Ferramentas

- Visual Studio Build Tools com **Desenvolvimento para desktop com C++** e Windows SDK.
- **CMake 3.24 ou superior**.
- Git para clonar o repositório, ou o código-fonte extraído.
- Internet durante a configuração inicial para obter as dependências de build.

No **x64 Native Tools Command Prompt for VS**:

```cmd
git clone https://github.com/BrendoGarcia/FeatherPad.git
cd FeatherPad
build.cmd
```

Ou execute diretamente:

```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

A saída fica em `build\Release\FeatherPad.exe`, acompanhada de `SDL3.dll`. Preserve as licenças ao redistribuir.

O CMake usa dependências fixadas e verificadas por SHA-256: **SDL 3.4.18** e **ViGEmClient** no commit `b66d02d57e32cc8595369c53418b843e958649b4`. O runtime C++ e o cliente ViGEm são vinculados estaticamente; a SDL é distribuída como DLL.

### O que os testes verificam?

- Persistência, migração, Unicode, argumentos com aspas, favoritos e backup.
- Importação de imagens e limites do cache.
- Abertura de janelas, estado maximizado e foco visual.
- Captura de botões, cancelamento, conflitos e confirmação.
- Preferências globais e por jogo, conversão de botões, eixos e gatilhos.
- Leitura SDL usando um gamepad de teste local ao processo.

Os testes da CI não instalam o driver virtual. Portanto, não substituem testes físicos de Bluetooth/USB, reconhecimento em jogos, instalação do ViGEmBus ou medição de memória no computador do usuário.

## 🩺 Solução de problemas

| Sintoma | O que conferir |
|---|---|
| Falta `SDL3.dll` ao ativar compatibilidade | Extraia todos os arquivos do pacote, mantendo a DLL ao lado do EXE. |
| O controle liga, mas o launcher não aparece | Confira se o monitor está na bandeja e se o botão de abertura está correto. Ative a inicialização com Windows se desejar. |
| O controle navega no launcher, mas não no jogo | Ative compatibilidade, confira o driver/status e escolha um formato aceito pelo jogo. Reinicie o jogo após alterar o formato. |
| Driver ausente ou saída virtual indisponível | Instale o ViGEmBus oficial, reinicie se solicitado e use **Verificar novamente**. |
| O controle não aparece na lista de origem | Confira USB/pareamento, feche remapeadores e verifique se há um mapeamento SDL adequado. |
| Comandos duplicados ou dois jogadores respondendo | Evite múltiplos conversores e consulte a configuração opcional do HidHide. |
| Modo Nativo não recebe entrada | Se configurou HidHide, confira se o controle físico está oculto para o jogo. |
| Botão errado abre/confirma | Use **Testar os botões do controle** e refaça a captura da ação. |
| A biblioteca não toma o foco | O Windows pode limitar a ativação de janelas; abra pelo ícone na bandeja. Outros apps também podem responder ao PS/Guide. |
| O jogo não abre | Confira caminho, argumentos, pasta de trabalho e necessidade do cliente da loja. |
| A imagem não é importada | Confira formato, tamanho máximo de 8 MB e dimensões de até 2048 × 2048. |
| Fechei o X e o programa continua aberto | É o comportamento esperado: use **Encerrar monitor** no menu da bandeja para sair. |
| Inicialização automática parou após mover o EXE | Desative e reative **Iniciar com o Windows** para registrar o novo caminho. |

Consulte também o [guia operacional detalhado](LEIA-ME.md).

Ao [abrir uma issue](https://github.com/BrendoGarcia/FeatherPad/issues), informe versão/commit, versão do Windows, modelo do controle, USB ou Bluetooth, modo de saída, mensagem de status e passos para reproduzir. Evite anexar sua biblioteca inteira sem revisar caminhos e dados pessoais.

## 🤝 Código, contribuições e licença

### Organização

| Arquivo/pasta | Responsabilidade |
|---|---|
| `src/experience.cpp` | Pesquisa, teclado virtual e tela cheia. |
| `src/UpdateManager.cpp` | Consulta HTTPS e download das atualizações. |
| `src/Updater.cpp` | Substituição dos arquivos, recuperação e reinicialização. |
| `src/main.cpp` | Inicialização, instância única e loop de mensagens. |
| `src/ui.cpp` | Biblioteca, formulários, foco e navegação. |
| `src/input.cpp` | Entrada HID/Raw Input. |
| `src/storage.cpp` | Persistência, migração, abertura de jogos e inicialização com Windows. |
| `src/art.cpp` | Importação de imagens, miniaturas e cache. |
| `src/controller_state.cpp` | Estado padronizado e tradução de entradas. |
| `src/gamepad_bridge.cpp` | SDL, dispositivos virtuais e ciclo de vida da compatibilidade. |
| `tests/` | Testes de regressão e integração nativa. |
| `.github/workflows/build.yml` | Build e testes automatizados no Windows. |

Contribuições são bem-vindas. Para mudanças de código, descreva o problema, mantenha o foco em leveza e acessibilidade pelo controle e execute os testes. Mudanças na entrada devem ser verificadas em hardware quando possível; registre o que foi efetivamente testado.

### Créditos

Projeto de [Brendo Garcia](https://github.com/BrendoGarcia).

Construído sobre as APIs nativas do Windows, [SDL](https://www.libsdl.org/) e [ViGEmClient/ViGEmBus](https://github.com/nefarius/ViGEmBus). O [HidHide](https://github.com/nefarius/HidHide) é uma ferramenta externa opcional para lidar com entrada dupla.

### Licença

O FeatherPad é distribuído sob a [licença MIT](LICENSE). Dependências de terceiros mantêm suas próprias licenças, incluídas na pasta `licenses` dos pacotes compilados.

---

<div align="center">

**Feito para escolher um jogo, pegar o controle e jogar.** 🎮

</div>
