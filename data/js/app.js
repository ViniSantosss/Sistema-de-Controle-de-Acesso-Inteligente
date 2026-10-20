const usuarioForm = document.getElementById("usuarioForm");

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
                    <div class="user-status">
                        <span>${usuario.ativo ? "Ativo" : "Desativado"}</span>

                        <div class="user-actions">
                            ${
                                usuario.ativo
                                    ? `
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
                    </div>
                </td>
            `;

            lista.appendChild(linha);
        });

    } catch (erro) {
        console.error("Erro ao carregar usuários:", erro);
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


carregarUsuarios();
carregarPainel();