export async function getJsonRest<T>(path: string, data : T) {
    try {
        const response = await fetch(path, {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json'
            }
        });
        let json = await response.json();
        data = json;
    } catch (error) {
        console.error('Error:', error);
    }
    return data;
}

export async function postJsonRest<T>(path: string, data: T) {
    try {
        const response = await fetch(path, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(data)
        });
        if (response.status == 200) {
            data = await response.json();
            // console.log(data);
        } else {
            console.error('Wrong Path.', 3000);
        }
    } catch (error) {
        console.error('Error: ' + error, 3000);
    }
    return data;
}