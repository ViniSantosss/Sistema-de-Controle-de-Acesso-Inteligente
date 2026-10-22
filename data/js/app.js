const usuarioForm = document.getElementById("usuarioForm");

const loginForm = document.getElementById("loginForm");

if (loginForm) {
    loginForm.addEventListener("submit", async function (event) {
        event.preventDefault();

        const mensagem = document.getElementById("mensagem");
        const usuario = document.getElementById("usuario").value;
        const senha = document.getElementById("senha").value;
        const endpoint = new URL("/api/login", window.location.href);

        try {
            const resposta = await fetch(endpoint, {
                method: "POST",
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8"
                },
                body: new URLSearchParams({ usuario, senha })
            });
            const corpo = await resposta.text();
            let dados;

            try {
                dados = JSON.parse(corpo);
            } catch (erro) {
                console.error("Resposta inesperada do servidor de login:", {
                    status: resposta.status,
                    contentType: resposta.headers.get("content-type"),
                    corpo: corpo.slice(0, 300),
                    erro
                });
                mensagem.textContent =
                    `O servidor em ${endpoint.origin} respondeu HTTP ${resposta.status} ` +
                    "sem JSON. Atualize o firmware e os arquivos LittleFS do ESP32; " +
                    "atualize a página e tente novamente.";
                return;
            }

            if (!resposta.ok) {
                mensagem.textContent = dados.erro || "Não foi possível entrar.";
                return;
            }

            window.location.replace("/panel.html");
        } catch (erro) {
            console.error("Erro ao realizar login:", erro);
            mensagem.textContent =
                `Não foi possível acessar ${endpoint.origin}/api/login. ` +
                "confirme que o dispositivo e seu navegador estão na mesma rede.";
        }
    });
}

if (usuarioForm) {
    const tipoAcessoInput = document.getElementById("tipoAcesso");
    const validadeCampos = document.getElementById("validadeCampos");
    const dataExpiracaoInput = document.getElementById("dataExpiracao");
    const usuarioIdInput = document.getElementById("usuarioId");
    const botaoCadastro = usuarioForm.querySelector('button[type="submit"]');
    const botaoCancelarEdicao = document.getElementById("cancelarEdicao");

    function atualizarCamposValidade() {
        const temporario = tipoAcessoInput.value.toUpperCase() === "TEMPORARIO";
        validadeCampos.hidden = !temporario;
        dataExpiracaoInput.required = temporario;
        if (!temporario) {
            dataExpiracaoInput.value = "";
        }
    }

    function cancelarEdicaoUsuario() {
        usuarioForm.reset();
        usuarioIdInput.value = "";
        botaoCadastro.textContent = "Cadastrar usuário";
        botaoCancelarEdicao.hidden = true;
        atualizarCamposValidade();
    }

    tipoAcessoInput.addEventListener("change", atualizarCamposValidade);
    botaoCancelarEdicao.addEventListener("click", cancelarEdicaoUsuario);
    atualizarCamposValidade();

    usuarioForm.addEventListener("submit", async function (event) {
        event.preventDefault();

        const nome = document.getElementById("nome").value;
        const tipoAcesso = tipoAcessoInput.value.toUpperCase();
        const usuarioId = usuarioIdInput.value;
        const parametros = new URLSearchParams({ nome, tipoAcesso });

        if (tipoAcesso === "TEMPORARIO") {
            parametros.set("dataExpiracao", dataExpiracaoInput.value);
        }

        if (usuarioId) {
            parametros.set("id", usuarioId);
        }

        try {
            const resposta = await fetch(
                `/api/usuarios?${parametros.toString()}`,
                {
                    method: usuarioId ? "PUT" : "POST"
                }
            );

            const dados = await resposta.json();

            const mensagem = document.getElementById("mensagem");

            if (!resposta.ok) {
                mensagem.textContent = dados.erro;
                return;
            }

            mensagem.textContent = dados.mensagem;

            cancelarEdicaoUsuario();
            await carregarUsuarios();

        } catch (erro) {
            console.error("Erro ao cadastrar usuário:", erro);
            document.getElementById("mensagem").textContent =
                "Não foi possível salvar o usuário. Verifique a conexão com o dispositivo.";
        }
    });
}

function escaparHtml(valor) {
    return String(valor).replace(/[&<>"']/g, caractere => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[caractere]);
}

async function carregarUsuarios() {
    try {
        const resposta = await fetch("/api/usuarios");

        const usuarios = await resposta.json();

        const lista = document.getElementById("listaUsuarios");

        if (!lista) {
            return;
        }

        lista.innerHTML = "";

        usuarios.forEach(usuario => {

            const linha = document.createElement("tr");
            const nome = escaparHtml(usuario.nome);
            const credencial = escaparHtml(usuario.credencialNfc || "Não cadastrada");
            const tipoAcesso = escaparHtml(usuario.tipoAcesso);
            const validade = usuario.tipoAcesso === "TEMPORARIO"
                ? usuario.dataExpiracaoFormatada
                    ? usuario.expirado
                        ? `Expirado em ${escaparHtml(usuario.dataExpiracaoFormatada)}`
                        : usuario.validadeVerificavel
                            ? `Válido até ${escaparHtml(usuario.dataExpiracaoFormatada)}`
                            : `Definido até ${escaparHtml(usuario.dataExpiracaoFormatada)}; horário não sincronizado`
                    : "Validade não configurada"
                : "Sem expiração";
            const statusUsuario = !usuario.ativo
                ? "Desativado"
                : usuario.expirado
                    ? "Expirado"
                    : usuario.tipoAcesso === "TEMPORARIO" &&
                        !usuario.validadeVerificavel
                        ? "Validade não verificável"
                    : "Ativo";

            linha.innerHTML = `
                <td>${nome}</td>

                <td>${credencial}</td>

                <td>${tipoAcesso}</td>

                <td>${validade}</td>

                <td>
                    <span class="${usuario.expirado ? "user-expired" : ""}">
                        ${statusUsuario}
                    </span>
                </td>

                <td class="user-actions-cell">
                    <div class="user-actions">
                        <button
                            type="button"
                            onclick="editarUsuario(${usuario.id})"
                        >
                            Editar
                        </button>

    ${
        usuario.ativo
            ? `
                <button
                    type="button"
                    onclick="atribuirChaveNFC(${usuario.id})"
                >
                    Atribuir chave
                </button>

                <button
                    type="button"
                    onclick="desativarUsuario(${usuario.id})"
                >
                    Desativar
                </button>
              `
            : ""
    }

    <button
        class="button-delete"
        type="button"
        onclick="excluirUsuario(${usuario.id})"
    >
        Excluir
    </button>

</div>
                </td>
            `;

            lista.appendChild(linha);
        });

    } catch (erro) {
        console.error("Erro ao carregar usuários:", erro);
    }
}

async function editarUsuario(id) {
    try {
        const resposta = await fetch("/api/usuarios");
        if (!resposta.ok) {
            throw new Error(`Falha ao carregar usuários: HTTP ${resposta.status}`);
        }

        const usuarios = await resposta.json();
        const usuario = usuarios.find(item => item.id === id);
        if (!usuario) {
            throw new Error("Usuário não encontrado.");
        }

        document.getElementById("usuarioId").value = usuario.id;
        document.getElementById("nome").value = usuario.nome;
        document.getElementById("tipoAcesso").value =
            usuario.tipoAcesso.toLowerCase();
        document.getElementById("dataExpiracao").value =
            usuario.dataExpiracao || "";
        document.getElementById("tipoAcesso").dispatchEvent(
            new Event("change")
        );

        document.querySelector('#usuarioForm button[type="submit"]').textContent =
            "Salvar alterações";
        document.getElementById("cancelarEdicao").hidden = false;
        document.getElementById("usuarioForm").scrollIntoView({
            behavior: "smooth",
            block: "center"
        });
    } catch (erro) {
        console.error("Erro ao iniciar edição do usuário:", erro);
    }
}

async function atribuirChaveNFC(id) {

    try {

        const resposta = await fetch(
            `/api/usuarios/aguardar-chave?id=${id}`,
            {
                method: "POST"
            }
        );

        const dados = await resposta.json();

        if (!resposta.ok) {
            console.error(dados.erro);
            return;
        }

        alert(dados.mensagem);

    } catch (erro) {

        console.error(
            "Erro ao iniciar atribuição da chave NFC:",
            erro
        );

    }
}


async function desativarUsuario(id) {
    try {
        const resposta = await fetch(
            `/api/usuarios/desativar?id=${id}`,
            {
                method: "POST"
            }
        );

        const dados = await resposta.json();

        if (!resposta.ok) {
            console.error(dados.erro);
            return;
        }

        carregarUsuarios();

    } catch (erro) {
        console.error("Erro ao desativar usuário:", erro);
    }
}


async function excluirUsuario(id) {
    try {
        const resposta = await fetch(
            `/api/usuarios?id=${id}`,
            {
                method: "DELETE"
            }
        );

        const dados = await resposta.json();

        if (!resposta.ok) {
            console.error(dados.erro);
            return;
        }

        carregarUsuarios();

    } catch (erro) {
        console.error("Erro ao excluir usuário:", erro);
    }
}

async function carregarPainel() {
    const totalUsuarios = document.getElementById("totalUsuarios");
    const usuariosAtivos = document.getElementById("usuariosAtivos");
    const usuariosDesativados = document.getElementById("usuariosDesativados");

    try {
        const respostaUsuarios = await fetch("/api/usuarios");
        if (!respostaUsuarios.ok) {
            throw new Error(`Falha ao carregar usuários: HTTP ${respostaUsuarios.status}`);
        }
        const usuarios = await respostaUsuarios.json();

        const total = usuarios.length;
        const ativos = usuarios.filter(usuario => usuario.ativo).length;
        const desativados = usuarios.filter(usuario => !usuario.ativo).length;

        if (totalUsuarios) {
            totalUsuarios.textContent = total;
        }

        if (usuariosAtivos) {
            usuariosAtivos.textContent = ativos;
        }

        if (usuariosDesativados) {
            usuariosDesativados.textContent = desativados;
        }
    } catch (erro) {
        console.error("Erro ao carregar usuários do painel:", erro);
    }
}

async function carregarStatusRede() {
    const wifiStatus = document.getElementById("wifiStatus");
    const painelWifi = document.getElementById("painelWifi");
    const wifiSsid = document.getElementById("wifiSsid");
    const painelSsid = document.getElementById("painelSsid");
    const ip = document.getElementById("ip");
    const painelIp = document.getElementById("painelIp");

    try {
        const respostaStatus = await fetch("/api/status");
        if (!respostaStatus.ok) {
            throw new Error(`Falha ao carregar status da rede: HTTP ${respostaStatus.status}`);
        }
        const status = await respostaStatus.json();

        const estadoWifi = status.wifi ? "Conectado" : "Desconectado";
        if (painelWifi) {
            painelWifi.textContent = estadoWifi;
        }
        if (wifiStatus) {
            wifiStatus.textContent = estadoWifi;
        }
        const nomeRede = status.wifi && status.ssid
            ? status.ssid
            : "Não conectado";
        if (wifiSsid) {
            wifiSsid.textContent = nomeRede;
        }
        if (painelSsid) {
            painelSsid.textContent = nomeRede;
        }
        if (painelIp) {
            painelIp.textContent = status.ip;
        }
        if (ip) {
            ip.textContent = status.ip;
        }
    } catch (erro) {
        console.error("Erro ao carregar status da rede:", erro);
    }
}

const deepSleepButton = document.getElementById("ativarDeepSleep");

if (deepSleepButton) {
    const deepSleepStatus = document.getElementById("deepSleepStatus");
    const deepSleepMessage = document.getElementById("deepSleepMessage");
    const deepSleepControl = document.getElementById("deepSleepControl");

    deepSleepButton.addEventListener("click", async function () {
        deepSleepButton.disabled = true;
        deepSleepButton.textContent = "Ativando Deep Sleep...";
        deepSleepMessage.textContent = "";
        deepSleepMessage.classList.remove("is-error");

        try {
            const resposta = await fetch("/api/deep-sleep", {
                method: "POST"
            });
            const dados = await resposta.json();

            if (!resposta.ok) {
                throw new Error(dados.erro || "Não foi possível ativar o Deep Sleep.");
            }

            deepSleepStatus.classList.remove("is-off");
            deepSleepStatus.classList.add("is-on");
            deepSleepStatus.innerHTML = `
                <span class="deep-sleep-status-dot" aria-hidden="true"></span>
                Ligado
            `;
            deepSleepControl.classList.add("is-on");
            deepSleepButton.textContent = "Deep Sleep ligado";
            deepSleepMessage.textContent = "Para sair do Deep Sleep, reinicie o ESP32.";
        } catch (erro) {
            console.error("Erro ao ativar Deep Sleep:", erro);
            deepSleepButton.disabled = false;
            deepSleepButton.textContent = "Ativar Deep Sleep";
            deepSleepMessage.textContent = erro.message;
            deepSleepMessage.classList.add("is-error");
        }
    });
}

async function carregarPerformance() {
    try {
        const resposta = await fetch("/api/performance");
        const performance = await resposta.json();

        const tempoResposta = document.getElementById("tempoResposta");
        const core0 = document.getElementById("core0");
        const core1 = document.getElementById("core1");
        const flash = document.getElementById("flash");
        const ram = document.getElementById("ram");
        
        if (tempoResposta) {
            tempoResposta.textContent = performance.tempoResposta.toFixed(1);
        }
        
        if (core0) {
            core0.textContent = performance.core0.toFixed(1);
        }

        if (core1) {
            core1.textContent = performance.core1.toFixed(1);
        }

        if (flash) {
            const usadaMB =
                performance.flashUsada / 1024 / 1024;

            const totalMB =
                performance.flashTotal / 1024 / 1024;

            const porcentagem =
                (performance.flashUsada /
                    performance.flashTotal) * 100;

            flash.textContent =
                `${usadaMB.toFixed(2)} MB / ` +
                `${totalMB.toFixed(2)} MB ` +
                `(${porcentagem.toFixed(1)}%)`;
        }

        if (ram) {
            const usadaMB =
                performance.ramUsada / 1024 / 1024;

            const totalMB =
                performance.ramTotal / 1024 / 1024;

            const porcentagem =
                (performance.ramUsada /
                    performance.ramTotal) * 100;

            ram.textContent =
                `${usadaMB.toFixed(2)} MB / ` +
                `${totalMB.toFixed(2)} MB ` +
                `(${porcentagem.toFixed(1)}%)`;
        }

    } catch (erro) {
        console.error("Erro ao carregar performance:", erro);
    }
}

if (document.getElementById("tempoResposta")) {
    carregarPerformance();
    setInterval(carregarPerformance, 1000);
}

const doorHistoryCanvas = document.getElementById("doorHistoryChart");

if (doorHistoryCanvas) {
    const doorHistoryMessage = document.getElementById("doorHistoryMessage");
    const doorCurrentState = document.getElementById("doorCurrentState");
    let doorHistoryData = null;

    function desenharHistoricoPorta() {
        if (!doorHistoryData || !doorHistoryData.horarioConfiavel) {
            return;
        }

        const contexto = doorHistoryCanvas.getContext("2d");
        const largura = doorHistoryCanvas.clientWidth;
        const altura = doorHistoryCanvas.clientHeight;
        if (!contexto || largura === 0 || altura === 0) {
            return;
        }

        const escala = window.devicePixelRatio || 1;
        doorHistoryCanvas.width = largura * escala;
        doorHistoryCanvas.height = altura * escala;
        contexto.scale(escala, escala);
        contexto.clearRect(0, 0, largura, altura);

        const margem = { esquerda: 58, direita: 14, superior: 12, inferior: 34 };
        const graficoLargura = largura - margem.esquerda - margem.direita;
        const graficoAltura = altura - margem.superior - margem.inferior;
        const agora = doorHistoryData.agora;
        const inicio = agora - 24 * 60 * 60;
        const eventos = doorHistoryData.eventos
            .filter(evento =>
                Number.isFinite(evento.timestamp) &&
                typeof evento.portaAberta === "boolean"
            )
            .sort((a, b) => a.timestamp - b.timestamp);
        const anteriores = eventos.filter(evento => evento.timestamp <= inicio);
        const recentes = eventos.filter(evento => evento.timestamp > inicio);
        const estadoInicial = anteriores.length
            ? anteriores[anteriores.length - 1]
            : null;
        const yEstado = aberta =>
            margem.superior + (aberta ? 0 : graficoAltura);
        const xTempo = timestamp =>
            margem.esquerda +
            ((timestamp - inicio) / (24 * 60 * 60)) * graficoLargura;

        contexto.font = "12px sans-serif";
        contexto.textBaseline = "middle";
        contexto.fillStyle = "#71858a";
        contexto.strokeStyle = "#dce9e7";
        contexto.lineWidth = 1;

        [true, false].forEach(aberta => {
            const y = yEstado(aberta);
            contexto.beginPath();
            contexto.moveTo(margem.esquerda, y);
            contexto.lineTo(largura - margem.direita, y);
            contexto.stroke();
            contexto.textAlign = "right";
            contexto.fillText(aberta ? "Aberta" : "Fechada", margem.esquerda - 9, y);
        });

        contexto.textAlign = "center";
        contexto.textBaseline = "top";
        for (let hora = 0; hora <= 24; hora += 6) {
            const x = margem.esquerda + (hora / 24) * graficoLargura;
            const instante = new Date((inicio + hora * 60 * 60) * 1000);
            const rotulo = instante.toLocaleTimeString("pt-BR", {
                hour: "2-digit",
                minute: "2-digit",
                timeZone: "America/Sao_Paulo"
            });
            contexto.beginPath();
            contexto.moveTo(x, margem.superior);
            contexto.lineTo(x, altura - margem.inferior);
            contexto.stroke();
            contexto.fillText(rotulo, x, altura - margem.inferior + 8);
        }

        const pontos = [];
        if (estadoInicial) {
            pontos.push({ timestamp: inicio, portaAberta: estadoInicial.portaAberta });
        }
        pontos.push(...recentes);

        if (pontos.length === 0) {
            return;
        }

        contexto.strokeStyle = "#278a70";
        contexto.lineWidth = 3;
        contexto.lineJoin = "round";
        contexto.beginPath();
        let anterior = pontos[0];
        contexto.moveTo(xTempo(anterior.timestamp), yEstado(anterior.portaAberta));

        for (let i = 1; i < pontos.length; i++) {
            const atual = pontos[i];
            contexto.lineTo(xTempo(atual.timestamp), yEstado(anterior.portaAberta));
            contexto.lineTo(xTempo(atual.timestamp), yEstado(atual.portaAberta));
            anterior = atual;
        }

        contexto.lineTo(xTempo(agora), yEstado(anterior.portaAberta));
        contexto.stroke();
    }

    async function carregarHistoricoPorta() {
        try {
            const resposta = await fetch("/api/historico-sensor", { cache: "no-store" });
            const dados = await resposta.json();
            if (!resposta.ok) {
                throw new Error(dados.erro || "Não foi possível carregar o histórico.");
            }

            if (!dados.disponivel) {
                throw new Error("O armazenamento do histórico não está disponível.");
            }

            doorHistoryData = dados;
            if (!dados.horarioConfiavel) {
                doorHistoryMessage.textContent =
                    "Histórico indisponível: o relógio do ESP32 ainda não foi sincronizado. Nenhum horário foi estimado.";
                doorCurrentState.textContent = "";
                const contexto = doorHistoryCanvas.getContext("2d");
                contexto.clearRect(0, 0, doorHistoryCanvas.width, doorHistoryCanvas.height);
                return;
            }

            const eventos = dados.eventos.filter(evento =>
                Number.isFinite(evento.timestamp) &&
                typeof evento.portaAberta === "boolean"
            );
            const estadoAtual = eventos.length
                ? eventos[eventos.length - 1].portaAberta
                : null;
            doorCurrentState.textContent = estadoAtual === null
                ? "Ainda não há medições com horário confiável."
                : `Último estado registrado: porta ${estadoAtual ? "aberta" : "fechada"}.`;

            if (!eventos.length) {
                doorHistoryMessage.textContent =
                    "Ainda não há histórico suficiente. Aguardando a primeira leitura confiável do sensor.";
            } else if (!dados.completo) {
                doorHistoryMessage.textContent =
                    "Histórico parcial: o limite local de armazenamento foi atingido e pode haver mudanças não exibidas.";
            } else if (eventos.length < 2) {
                doorHistoryMessage.textContent =
                    "Histórico inicial: ainda não há mudanças de estado suficientes; a linha mostra a última leitura.";
            } else {
                doorHistoryMessage.textContent =
                    "Histórico das últimas 24 horas, preservado no dispositivo após reinicializações.";
            }

            desenharHistoricoPorta();
        } catch (erro) {
            console.error("Erro ao carregar histórico do sensor:", erro);
            doorHistoryMessage.textContent = erro.message;
            doorCurrentState.textContent = "";
        }
    }

    carregarHistoricoPorta();
    setInterval(carregarHistoricoPorta, 30000);
    window.addEventListener("resize", desenharHistoricoPorta);
}

const otaForm = document.getElementById("otaForm");

if (otaForm) {
    const otaFirmware = document.getElementById("otaFirmware");
    const otaSubmit = document.getElementById("otaSubmit");
    const otaProgress = document.getElementById("otaProgress");
    const otaMessage = document.getElementById("otaMessage");

    otaForm.addEventListener("submit", function (event) {
        event.preventDefault();

        const arquivo = otaFirmware.files[0];
        if (!arquivo) {
            otaMessage.textContent = "Selecione um arquivo de firmware .bin.";
            otaMessage.classList.add("is-error");
            return;
        }

        if (!arquivo.name.toLowerCase().endsWith(".bin")) {
            otaMessage.textContent = "O arquivo selecionado deve ter extensão .bin.";
            otaMessage.classList.add("is-error");
            return;
        }

        const corpo = new FormData();
        corpo.append("firmware", arquivo);

        const requisicao = new XMLHttpRequest();
        requisicao.open("POST", "/api/ota");
        requisicao.withCredentials = true;

        otaSubmit.disabled = true;
        otaProgress.hidden = false;
        otaProgress.value = 0;
        otaMessage.classList.remove("is-error");
        otaMessage.textContent = "Enviando e gravando firmware...";

        requisicao.upload.addEventListener("progress", evento => {
            if (evento.lengthComputable) {
                otaProgress.value = (evento.loaded / evento.total) * 100;
                otaMessage.textContent =
                    `Enviando firmware: ${otaProgress.value.toFixed(0)}%.`;
            }
        });

        requisicao.addEventListener("load", () => {
            let dados;
            try {
                dados = JSON.parse(requisicao.responseText);
            } catch (erro) {
                console.error("Resposta OTA inválida:", erro);
                otaMessage.textContent =
                    `Resposta inválida do ESP32 (HTTP ${requisicao.status}).`;
                otaMessage.classList.add("is-error");
                otaSubmit.disabled = false;
                return;
            }

            if (requisicao.status < 200 || requisicao.status >= 300) {
                otaMessage.textContent =
                    dados.erro || `Falha na atualização (HTTP ${requisicao.status}).`;
                otaMessage.classList.add("is-error");
                otaSubmit.disabled = false;
                return;
            }

            otaProgress.value = 100;
            otaMessage.textContent =
                `${dados.mensagem} Aguarde o dispositivo reiniciar e recarregue a página.`;
        });

        requisicao.addEventListener("error", () => {
            otaMessage.textContent =
                "Falha de comunicação durante a atualização. Verifique se o ESP32 ainda está acessível.";
            otaMessage.classList.add("is-error");
            otaSubmit.disabled = false;
        });

        requisicao.addEventListener("abort", () => {
            otaMessage.textContent = "Envio da atualização cancelado.";
            otaMessage.classList.add("is-error");
            otaSubmit.disabled = false;
        });

        requisicao.send(corpo);
    });
}

if (document.getElementById("listaUsuarios")) {
    carregarUsuarios();
    setInterval(carregarUsuarios, 60000);
}

if (document.getElementById("totalUsuarios")) {
    carregarPainel();
}

if (
    document.getElementById("wifiStatus") ||
    document.getElementById("painelWifi") ||
    document.getElementById("wifiSsid") ||
    document.getElementById("painelSsid")
) {
    carregarStatusRede();
}
