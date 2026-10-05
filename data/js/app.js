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