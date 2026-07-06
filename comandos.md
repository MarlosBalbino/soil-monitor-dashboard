# Resumo de Comandos e Configurações

Este documento reúne os principais comandos e locais de configuração utilizados no projeto **Soil Monitor**.

---

## Zephyr (ESP32-WROVER)

### Compilar

```bash
west build -p always -b esp_wrover_kit/esp32/procpu
```

### Gravar firmware

```bash
west flash --erase
```

### Monitor serial

```bash
west espressif monitor
```

### Configurar Wi-Fi

```text
conectar_wifi brisa-2789486 ytokjmue

conectar_wifi vaca vaca12345

conectar_wifi mbn-note note12345

conectar_wifi <SSID> <SENHA>
```

---

## Servidor (Flask)

### Iniciar servidor

```bash
python server.py
```

---

## Dashboard (Next.js)

### Instalar dependências

```bash
npm install
```

### Executar

```bash
npm run dev -- --hostname 192.168.0.5

npm run dev -- --hostname 10.138.5.112

npm run dev -- --hostname <IP_DO_PC>
```

### Descobrir o IP do computador

```bash
ipconfig
```

---

## Configuração do IP do Servidor (API Flask)

O endereço IP do servidor Flask deve ser configurado nos seguintes locais:

| Componente | Arquivo |
|------------|----------|
| ESP32 (Zephyr) | `prj.config` |
| Dashboard (Next.js) | `.env.local` |

---

## Fluxo da Aplicação

```text
ESP32 --HTTP POST--> Flask Server --HTTP GET--> Dashboard
```