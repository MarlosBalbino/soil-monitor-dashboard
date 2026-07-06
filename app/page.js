"use client";

import styles from "./page.module.css";

import { useEffect, useState } from "react";

import {
    LineChart,
    Line,
    CartesianGrid,
    XAxis,
    YAxis,
    Tooltip,
    ResponsiveContainer
} from "recharts";

export default function Home() {

    const interval = 20;
    const poolingRate = 2000;
    const [historico, setHistorico] = useState([]);

    const [isMobile, setIsMobile] = useState(false);

    useEffect(() => {

      function handleResize() {
          setIsMobile(window.innerWidth < 768);
      }
          handleResize();

          window.addEventListener("resize", handleResize);

          return () => window.removeEventListener("resize", handleResize);

    }, []);

    async function atualizar() {

        try {

            const response = await fetch("/api/sensor");

            const json = await response.json();

            if (json.error) return;

            setHistorico(old => {

                const hora = new Date().toLocaleTimeString();

                const novo = [
                    ...old,
                    {
                        hora,
                        temperatura: json.temperatura,
                        umidade: json.umidade,
                        irrigando: json.irrigamento
                    }
                ];

                if (novo.length > interval)
                    novo.shift();

                return novo;

            });

        }

        catch (e) {
            console.log(e);
        }

    }

    useEffect(() => {

        atualizar();

        const interval = setInterval(atualizar, poolingRate);

        return () => clearInterval(interval);

    }, []);

    const ultimo = historico[historico.length - 1];

    return (

        <main className={styles.body}>

            <h1 className={styles.title}>
                🌱 Dashboard da Irrigação
            </h1>

            <div className={styles.cards}>

                <Card
                    titulo="Temperatura"
                    valor={
                        ultimo
                            ? `${ultimo.temperatura.toFixed(1)} °C`
                            : "--"
                    }
                    cor="#ef4444"
                />

                <Card
                    titulo="Umidade"
                    valor={
                        ultimo
                            ? `${ultimo.umidade.toFixed(1)} %`
                            : "--"
                    }
                    cor="#2563eb"
                />

                <Card
                    titulo="Irrigação"
                    valor={
                        ultimo
                            ? ultimo.irrigando
                                ? "Ligada"
                                : "Desligada"
                            : "--"
                    }
                    cor={
                        ultimo?.irrigando
                            ? "#16a34a"
                            : "#6b7280"
                    }
                />

            </div>

            <div className={styles.graficos}>

                <div className={styles.chartCard}>

                    <h2>Temperatura</h2>

                    <ResponsiveContainer
                      width="100%"
                      height={isMobile ? 200 : 300}
                    >

                        <LineChart data={historico}>

                            <CartesianGrid strokeDasharray="3 3"/>

                            <XAxis dataKey="hora"/>

                            <YAxis domain={[20, 40]} />

                            <Tooltip
                                contentStyle={{
                                    backgroundColor: "rgba(255,255,255,0.75)",
                                    border: "1px solid #ddd",
                                    borderRadius: 10,
                                    padding: isMobile ? "5px 8px" : "8px 12px",
                                    fontSize: isMobile ? 11 : 13,
                                    boxShadow: "0 2px 8px rgba(0,0,0,0.15)",
                                    backdropFilter: "blur(6px)"
                                }}
                                labelStyle={{
                                    fontSize: isMobile ? 10 : 12,
                                    fontWeight: 600
                                }}
                                itemStyle={{
                                    fontSize: isMobile ? 10 : 12,
                                    padding: 0
                                }}
                            />

                            <Line
                                type="linear"
                                dataKey="temperatura"
                                stroke="#ef4444"
                                strokeWidth={2}
                                dot={true}
                                isAnimationActive={false}
                            />

                        </LineChart>

                    </ResponsiveContainer>

                </div>

                <div className={styles.chartCard}>

                    <h2>Umidade</h2>

                    <ResponsiveContainer
                      width="100%"
                      height={isMobile ? 200 : 300}
                    >

                        <LineChart data={historico}>

                            <CartesianGrid strokeDasharray="3 3"/>

                            <XAxis dataKey="hora"/>

                            <YAxis/>

                            <Tooltip
                                contentStyle={{
                                    backgroundColor: "rgba(255,255,255,0.75)",
                                    border: "1px solid #ddd",
                                    borderRadius: 10,
                                    padding: isMobile ? "5px 8px" : "8px 12px",
                                    fontSize: isMobile ? 11 : 13,
                                    boxShadow: "0 2px 8px rgba(0,0,0,0.15)",
                                    backdropFilter: "blur(6px)"
                                }}
                                labelStyle={{
                                    fontSize: isMobile ? 10 : 12,
                                    fontWeight: 600
                                }}
                                itemStyle={{
                                    fontSize: isMobile ? 10 : 12,
                                    padding: 0
                                }}
                            />

                            <Line
                                type="linear"
                                dataKey="umidade"
                                stroke="#2563eb"
                                strokeWidth={2}
                                dot={true}
                                isAnimationActive={false}
                            />

                        </LineChart>

                    </ResponsiveContainer>

                </div>

            </div>

        </main>

    );

}

function Card({ titulo, valor, cor }) {

    return (
        <div className={styles.card}>

            <h3>{titulo}</h3>

            <h1
                className={styles.cardValue}
                style={{ color: cor }}
            >
                {valor}
            </h1>

        </div>
    );

}