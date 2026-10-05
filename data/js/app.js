async function carregarStatus() {
    try {
        const resposta = await fetch("/api/status");

        const dados = await resposta.json();

        document.getElementById("status").textContent = dados.status;
        document.getElementById("wifi").textContent =
            dados.wifi ? "Conectado" : "Desconectado";
        document.getElementById("ip").textContent = dados.ip;

    } catch (erro) {
        console.error("Erro ao buscar status:", erro);

        document.getElementById("status").textContent = "Erro";
        document.getElementById("wifi").textContent = "Erro";
        document.getElementById("ip").textContent = "Erro";
    }
}

carregarStatus();

async function ligarLed() {
    try {
        const resposta = await fetch("/api/led/on");
        const dados = await resposta.json();

        if (dados.led) {
            document.getElementById("ledStatus").textContent = "Ligado";
        }
    } catch (erro) {
        console.error("Erro ao ligar LED:", erro);
    }
}

async function desligarLed() {
    try {
        const resposta = await fetch("/api/led/off");
        const dados = await resposta.json();

        if (!dados.led) {
            document.getElementById("ledStatus").textContent = "Desligado";
        }
    } catch (erro) {
        console.error("Erro ao desligar LED:", erro);
    }
}