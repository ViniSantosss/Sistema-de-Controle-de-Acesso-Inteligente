async function carregarStatus() {
    try {
        const resposta = await fetch("/api/status");
        if (!resposta.ok) throw new Error(`HTTP ${resposta.status}`);

        const dados = await resposta.json();

        document.getElementById("status").textContent = dados.status;
        document.getElementById("wifi").textContent =
            dados.wifi ? "Conectado" : "Desconectado";
        document.getElementById("ip").textContent = dados.ip;
        document.getElementById("status-indicator").classList.toggle("is-offline", dados.status !== "online");

    } catch (erro) {
        console.error("Erro ao buscar status:", erro);

        document.getElementById("status").textContent = "Erro";
        document.getElementById("wifi").textContent = "Erro";
        document.getElementById("ip").textContent = "Erro";
        document.getElementById("status-indicator").classList.add("is-offline");
    }
}

async function controlarLed(acao) {
    const botoes = document.querySelectorAll(".action-button");
    const statusLed = document.getElementById("ledStatus");
    const indicadorLed = document.getElementById("led-indicator");
    const mensagem = document.getElementById("action-message");
    const ligar = acao === "on";

    botoes.forEach((botao) => { botao.disabled = true; });
    mensagem.textContent = "";
    statusLed.textContent = "Atualizando...";

    try {
        const resposta = await fetch(`/api/led/${acao}`);
        if (!resposta.ok) throw new Error(`HTTP ${resposta.status}`);

        const dados = await resposta.json();
        if (dados.led !== ligar) throw new Error("Resposta inesperada do dispositivo");

        statusLed.textContent = dados.led ? "Ligado" : "Desligado";
        indicadorLed.classList.toggle("is-on", dados.led);
    } catch (erro) {
        console.error(`Erro ao ${ligar ? "ligar" : "desligar"} LED:`, erro);
        statusLed.textContent = "Indisponível";
        mensagem.textContent = "Não foi possível atualizar o LED. Verifique a conexão e tente novamente.";
    } finally {
        botoes.forEach((botao) => { botao.disabled = false; });
    }
}

document.getElementById("turn-on").addEventListener("click", () => controlarLed("on"));
document.getElementById("turn-off").addEventListener("click", () => controlarLed("off"));

carregarStatus();