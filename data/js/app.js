const usuarioForm = document.getElementById("usuarioForm");

const loginForm = document.getElementById("loginForm");

if (loginForm) {
    loginForm.addEventListener("submit", async function (event) {
        event.preventDefault();

        const mensagem = document.getElementById("mensagem");
        const usuario = document.getElementById("usuario").value;
        const senha = document.getElementById("senha").value;

        try {
            const resposta = await fetch("/api/login", {
                method: "POST",
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8"
                },
                body: new URLSearchParams({ usuario, senha })
            });
            const dados = await resposta.json();

            if (!resposta.ok) {
                mensagem.textContent = dados.erro || "Não foi possível entrar.";
                return;
            }

            window.location.replace("/panel.html");
        } catch (erro) {
            console.error("Erro ao realizar login:", erro);
            mensagem.textContent = "Não foi possível conectar ao servidor.";
        }
    });
}

if (usuarioForm) {
    usuarioForm.addEventListener("submit", async function (event) {
        event.preventDefault();

        const nome = document.getElementById("nome").value;
        const tipoAcesso = document
            .getElementById("tipoAcesso")
            .value
            .toUpperCase();

        try {
            const resposta = await fetch(
                `/api/usuarios?nome=${encodeURIComponent(nome)}&tipoAcesso=${encodeURIComponent(tipoAcesso)}`,
                {
                    method: "POST"
                }
            );

            const dados = await resposta.json();

            const mensagem = document.getElementById("mensagem");

            if (!resposta.ok) {
                mensagem.textContent = dados.erro;
                return;
            }

            mensagem.textContent = dados.mensagem;

            usuarioForm.reset();

            carregarUsuarios();

        } catch (erro) {
            console.error("Erro ao cadastrar usuário:", erro);
        }
    });
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

            linha.innerHTML = `
                <td>${usuario.nome}</td>

                <td>
                    ${usuario.credencialNfc || "Não cadastrada"}
                </td>

                <td>
                    ${usuario.tipoAcesso}
                </td>

                <td>
                    <div class="user-actions">

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
    try {
        const respostaUsuarios = await fetch("/api/usuarios");
        const usuarios = await respostaUsuarios.json();

        const total = usuarios.length;
        const ativos = usuarios.filter(usuario => usuario.ativo).length;
        const desativados = usuarios.filter(usuario => !usuario.ativo).length;

        const totalUsuarios = document.getElementById("totalUsuarios");
        const usuariosAtivos = document.getElementById("usuariosAtivos");
        const usuariosDesativados = document.getElementById("usuariosDesativados");

        if (totalUsuarios) {
            totalUsuarios.textContent = total;
        }

        if (usuariosAtivos) {
            usuariosAtivos.textContent = ativos;
        }

        if (usuariosDesativados) {
            usuariosDesativados.textContent = desativados;
        }

        const respostaStatus = await fetch("/api/status");
        const status = await respostaStatus.json();

        const statusSistema = document.getElementById("statusSistema");
        const painelWifi = document.getElementById("painelWifi");
        const painelIp = document.getElementById("painelIp");

        if (statusSistema) {
            statusSistema.textContent = status.status === "online"
                ? "Online"
                : "Offline";
        }

        if (painelWifi) {
            painelWifi.textContent = status.wifi
                ? "Conectado"
                : "Desconectado";
        }

        if (painelIp) {
            painelIp.textContent = status.ip;
        }

    } catch (erro) {
        console.error("Erro ao carregar painel:", erro);
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

if (document.getElementById("listaUsuarios")) {
    carregarUsuarios();
}

if (document.getElementById("totalUsuarios")) {
    carregarPainel();
}