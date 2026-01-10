const SCREEN_MAX_WIDTH  = 132;
const SCREEN_MAX_HEIGHT = 64;

//========================================

class Api {
	constructor() {
		this.base_url = "http://192.168.2.128:8016/api";
	}

	async get(uri) {
		const response = await fetch(`${this.base_url}${uri}`);
		if (!response.ok) {
			if (response.body) {
				alert(`request failed with status code ${response.status}: ${await response.text()}`);
			}

			else {
				alert(`request failed with status code ${response.status}`);
			}

			throw new Error("request failed");
		}

		return await response.json();
	}

	async get_measurements() {
		return await this.get("/get_measurements");
	}

	async get_screen_content() {
		return await this.get("/get_screen_content");
	}
}

//========================================

document.addEventListener("DOMContentLoaded", async () => {
	const api = new Api();

	async function update() {
		const result = await api.get_measurements();

		Array.from(document.getElementsByClassName("measurement-info")).forEach(element => {
			element.innerText = String(result[element.getAttribute("field")].toFixed(2)).padStart(5, "0");
		});
	}

	await update();
	window.setInterval(update, 1000);
});

//========================================