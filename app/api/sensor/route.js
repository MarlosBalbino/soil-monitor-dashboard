export async function GET() {
    try {
        const response = await fetch("http://10.138.5.112:8080/api/dados", {
            cache: "no-store"
        });

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


// export async function GET() {
//     try {
//         const response = await fetch("http://192.168.0.5:8080/api/dados", {
//             cache: "no-store"
//         });

//         if (!response.ok) {
//             throw new Error("Erro ao acessar Flask");
//         }

//         const data = await response.json();

//         return Response.json(data);

//     } catch (err) {

//         return Response.json({
//             error: err.message
//         }, {
//             status: 500
//         });

//     }
// }

