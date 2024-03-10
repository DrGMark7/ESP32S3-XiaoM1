const fs = require("fs");
const http = require("http");
const { url } = require("inspector");
const path = require("path"); 

const port = 8888;
const ip = "0.0.0.0";

const server = http.createServer((request, response) => {
    if (request.method === "POST" && request.url.startsWith("/uploadAudio")) {

        const fileName = path.basename(request.url).replace("uploadAudio","");
        var recordingFile = fs.createWriteStream(`./resources/${fileName}.wav`, { flags: 'a' }); // Use 'a' flag for appending data
        
        request.on("data", function(data) {
            recordingFile.write(data);
        });

        request.on("end", async function() {
            response.writeHead(200, { "Content-Type": "text/plain" });
            response.end("File uploaded successfully!");
            console.log("Write file sucessful")
        });
    } else {
        console.log("Error: Check your POST request");
        response.writeHead(405, { "Content-Type": "text/plain" });
        response.end("Method Not Allowed");
    }
});

server.listen(port, ip);
console.log(`Listening at ${ip}:${port}`);


// const fs = require("fs");
// const http = require("http");

// const port = 8888;
// const ip = "0.0.0.0";
// const fileName = "./resources/recording.wav";

// const server = http.createServer((request, response) => {
//     if (request.method === "POST" && request.url === "/uploadAudio") {
//         var recordingFile = fs.createWriteStream(fileName, { flags: 'a' }); // Use 'a' flag for appending data

//         request.on("data", function(data) {
//             recordingFile.write(data);
//         });

//         request.on("end", async function() {
//             response.writeHead(200, { "Content-Type": "audio/wav" });
//             response.end("File uploaded successfully!");
//         });
//     } else {
//         console.log("Error: Check your POST request");
//         response.writeHead(405, { "Content-Type": "audio/wav" });
//         response.end("Method Not Allowed");
//     }
// });

// server.listen(port, ip);
// console.log(`Listening at ${ip}:${port}`);
