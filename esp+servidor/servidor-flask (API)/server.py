from flask import Flask, request, jsonify
from flask_cors import CORS
from conversor import to_UR, to_celsius

app = Flask(__name__)
CORS(app)

# Memória global para persistir o último estado enviado pelo ESP32
ultimo_estado_sensores = {
    "umidade": 0.0,
    "temperatura": 0.0,
    "sistema_irrigando": False,
    "dados_disponiveis": False
}

# ====================================================================
# ROTA 1: Exclusiva para o ESP32 enviar os dados (POST)
# ====================================================================
@app.route('/api/dados', methods=['POST'])
def receber_dados_esp32():
    global ultimo_estado_sensores
    
    if request.is_json:
        dados = request.get_json()
        print(f"\n[ESP32] Novo pacote recebido!")
        
        raw_pot1 = dados.get('potenciometro1')
        raw_pot2 = dados.get('potenciometro2')
        status_irrigacao = dados.get('irrigando')
        
        umidade_convertida = to_UR(raw_pot1)
        temperatura_convertida = to_celsius(raw_pot2)
        
        # Atualiza o cache global do servidor
        ultimo_estado_sensores = {
            "umidade": umidade_convertida,
            "temperatura": temperatura_convertida,
            "sistema_irrigando": status_irrigacao,
            "dados_disponiveis": True
        }
        
        print(f"  -> Umidade: {umidade_convertida}% | Temperatura: {temperatura_convertida}°C | Irrigando: {status_irrigacao}")
        return jsonify({"status": "sucesso"}), 200
    else:
        return jsonify({"error": "Formato invalido"}), 400


# ====================================================================
# ROTA 2: Exclusiva para o seu Dashboard JavaScript (GET)
# ====================================================================
@app.route('/api/dados', methods=['GET'])
def obter_dados_frontend():
    # Se o Dashboard tentar ler e a ESP32 ainda não tiver enviado nada
    if not ultimo_estado_sensores["dados_disponiveis"]:
        # Retorna a chave 'error' que o seu JS usa para dar o 'return' de proteção
        return jsonify({"error": "Dados do sensor ainda nao disponiveis"}), 200
        
    # CORREÇÃO DA ESTRUTURA: Retorna exatamente o contrato que o seu Front-end espera
    return jsonify({
        "temperatura": ultimo_estado_sensores["temperatura"],
        "umidade": ultimo_estado_sensores["umidade"],
        "irrigamento": ultimo_estado_sensores["sistema_irrigando"] # Chave exata do seu json.irrigamento
    }), 200


if __name__ == '__main__':
    print("==================================================")
    print(" Servidor IoT iniciado com Rotas Independentes!")
    print(" -> ESP32 Envia para:   POST http://localhost:8080/api/dados")
    print(" -> Dashboard Lê de:    GET  http://localhost:8080/api/sensor")
    print("==================================================")
    app.run(host='0.0.0.0', port=8080, debug=True)