# Soil Monitor Dashboard

O **Soil Monitor Dashboard** é uma aplicação desenvolvida em **Next.js** responsável por exibir as medições enviadas pela ESP32 ao servidor Flask.

O Dashboard consulta periodicamente a API através de requisições **HTTP GET** e atualiza os gráficos e indicadores da interface.

---

## Pré-requisitos

- Node.js instalado.
- Dependências do projeto instaladas:

```bash
npm install
```

---

## Iniciando o servidor de desenvolvimento

Execute o comando abaixo informando o endereço IP do computador onde o Dashboard será executado:

```bash
npm run dev -- --hostname <IP_DO_COMPUTADOR>
```

### Exemplos

```bash
npm run dev -- --hostname 192.168.0.5
```

```bash
npm run dev -- --hostname 10.138.5.112
```

### Descobrindo o endereço IP

No **Prompt de Comando (CMD)** ou **PowerShell**, execute:

```bash
ipconfig
```

Utilize o endereço IPv4 da interface de rede conectada à mesma rede da ESP32.

> **Observação:** informar o parâmetro `--hostname` é recomendado quando outros serviços (como o WSL) estão em execução, garantindo que o servidor Next.js seja iniciado utilizando o endereço IP do computador hospedeiro.

---

## Acessando o Dashboard

Após iniciar o servidor, abra o navegador e acesse:

```text
http://<IP_DO_COMPUTADOR>:3000
```

Exemplo:

```text
http://192.168.0.5:3000
```

Caso esteja acessando a partir de um smartphone ou outro computador, certifique-se de que todos os dispositivos estejam conectados à mesma rede local.

---

## Configurando o endereço do servidor (API Flask)

O Dashboard obtém as medições realizando requisições **HTTP GET** para a API Flask.

Configure o endereço do servidor editando o arquivo:

```text
.env.local
```

Exemplo:

```env
FLASK_HOST=192.168.0.5
FLASK_PORT=8080
```

Após alterar as variáveis de ambiente, reinicie o servidor de desenvolvimento.

---

## Fluxo de comunicação

```text
ESP32-WROVER
      │
      │ HTTP POST
      ▼
+-------------------+
|   Flask Server    |
+-------------------+
      ▲
      │ HTTP GET
      │
Next.js Dashboard
```

O Dashboard apenas consome os dados disponibilizados pelo servidor. Toda a comunicação com a ESP32 é realizada pela API Flask.