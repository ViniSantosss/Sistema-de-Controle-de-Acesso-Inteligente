const loginForm = document.getElementById("loginForm");
const mensagem = document.getElementById("mensagem");

loginForm.addEventListener("submit", async (evento) => {

    evento.preventDefault();

    const usuario = document.getElementById("usuario").value;
    const senha = document.getElementById("senha").value;

    try {

        const resposta = await fetch(
            `/api/login?usuario=${encodeURIComponent(usuario)}&senha=${encodeURIComponent(senha)}`
        );

        const dados = await resposta.json();

        if (dados.autenticado) {
            mensagem.textContent = "Login realizado com sucesso!";
        } else {
            mensagem.textContent = "Usuário ou senha incorretos.";
        }

    } catch (erro) {

        console.error("Erro ao realizar login:", erro);
        mensagem.textContent = "Erro ao conectar com o ESP32.";

    }
});