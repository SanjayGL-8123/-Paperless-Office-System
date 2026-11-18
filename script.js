let files = JSON.parse(localStorage.getItem("files")) || [];

function saveFile() {
    let name = document.getElementById("fileName").value;
    let note = document.getElementById("fileNote").value;

    if (name === "" || note === "") {
        alert("Please fill all fields");
        return;
    }

    files.push({ name, note });
    localStorage.setItem("files", JSON.stringify(files));

    document.getElementById("fileName").value = "";
    document.getElementById("fileNote").value = "";

    displayFiles();
}

function displayFiles() {
    let list = document.getElementById("fileList");
    list.innerHTML = "";

    files.forEach((file, index) => {
        let li = document.createElement("li");
        li.innerHTML = `
            <strong>${file.name}</strong><br>
            ${file.note}<br>
            <button onclick="deleteFile(${index})">Delete</button>
        `;
        list.appendChild(li);
    });
}

function deleteFile(index) {
    files.splice(index, 1);
    localStorage.setItem("files", JSON.stringify(files));
    displayFiles();
}

displayFiles();
