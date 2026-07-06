# conversor.py

def to_UR(valor_potenciometro):
    """
    Converte o valor bruto do 1º potenciômetro para Umidade Relativa (UR).
    Calibração: 130 = 0% UR | 2371 = 100% UR
    """
    adc_min, adc_max = 130, 2371
    ur_min, ur_max = 0.0, 100.0
    
    # Evita erros caso o valor enviado seja nulo ou inválido
    if valor_potenciometro is None:
        return 0.0

    # Aplicação da fórmula de interpolação linear
    resultado = ur_min + ((float(valor_potenciometro - adc_min) * (ur_max - ur_min)) / (adc_max - adc_min))
    
    # Limitador (Clamping): Garante que ruídos não gerem valores menores que 0 ou maiores que 100
    resultado = max(ur_min, min(ur_max, resultado))
    
    return round(resultado, 1)


def to_celsius(valor_potenciometro):
    """
    Converte o valor bruto do 2º potenciômetro para Temperatura em Celsius (°C).
    Calibração: 130 = 0°C | 2371 = 60°C
    """
    adc_min, adc_max = 130, 2371
    temp_min, temp_max = 0.0, 60.0
    
    # Evita erros caso o valor enviado seja nulo ou inválido
    if valor_potenciometro is None:
        return 0.0

    # Aplicação da fórmula de interpolação linear
    resultado = temp_min + ((float(valor_potenciometro - adc_min) * (temp_max - temp_min)) / (adc_max - adc_min))
    
    # Limitador (Clamping): Garante que o valor fique estritamente entre 0°C e 60°C
    resultado = max(temp_min, min(temp_max, resultado))
    
    return round(resultado, 1)