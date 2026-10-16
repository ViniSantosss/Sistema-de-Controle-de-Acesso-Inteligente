const usuarioForm = document.getElementById("usuarioForm");

if (usuarioForm) {
    usuarioForm.addEventListener("submit", async function (event) {
        event.preventDefault();

        const nome = document.getElementById("nome").value;
        const tipoAcesso = document.getElementById("tipoAcesso").value.toUpperCase();

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
                <td>Não cadastrada</td>
                <td>${usuario.tipoAcesso}</td>
                <td>
                    ${usuario.ativo ? "Ativo" : "Desativado"}
                    ${
                        usuario.ativo
                            ? `<button type="button" onclick="desativarUsuario(${usuario.id})">
                                Desativar
                               </button>`
                            : ""
                    }
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

carregarUsuarios();