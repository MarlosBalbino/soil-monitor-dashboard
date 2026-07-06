const host = process.env.FLASK_HOST;
const port = process.env.FLASK_PORT;

export async function GET() {
    try {
        const response = await fetch(
            `http://${host}:${port}/api/dados`,
            {
                cache: "no-store",
            }
        );

        if (!response.ok) {
            throw new Error("Erro ao acessar Flask");
        }

        const data = await response.json();

        return Response.json(data);

    } catch (err) {

        return Response.json({
            error: err.message
        }, {
            status: 500
        });

    }
}
