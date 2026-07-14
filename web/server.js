const express = require('express');
const cors = require('cors');
const fs = require('fs');
const path = require('path');
const audioDuration = require('audio-duration');

const app = express();
const PORT = 8080;

app.use(cors());
app.use(express.json());
app.use(express.static('web'));

// Store current playback state
let currentPlaylist = [];
let currentIndex = 0;
let isPlaying = false;
let currentProgress = 0;
let currentSongDuration = 0;

// Load songs from /Melodify folder
function loadSongs() {
    const songsPath = path.join(__dirname);
    const files = fs.readdirSync(songsPath);
    const songFiles = files.filter(file => file.endsWith('.mp3'));
    
    return songFiles.map((file, index) => ({
        id: index,
        name: file.replace('.mp3', ''),
        file: file,
        path: path.join(songsPath, file)
    }));
}

let songs = loadSongs();

// API Routes
app.get('/api/playlist', (req, res) => {
    res.json({ songs: songs });
});

app.get('/api/userplaylist', (req, res) => {
    // For demo, return empty playlist
    // You can implement persistent storage if needed
    res.json({ songs: [] });
});

app.post('/api/addtoplaylist', (req, res) => {
    // For demo, just return success
    res.json({ success: true });
});

app.get('/api/play', (req, res) => {
    const songFile = req.query.file;
    const song = songs.find(s => s.file === songFile);
    
    if (song) {
        currentIndex = songs.findIndex(s => s.file === songFile);
        res.json({ success: true, song: song.name });
    } else {
        res.status(404).json({ error: 'Song not found' });
    }
});

app.get('/api/pause', (req, res) => {
    isPlaying = false;
    res.json({ success: true });
});

app.get('/api/resume', (req, res) => {
    isPlaying = true;
    res.json({ success: true });
});

app.get('/api/next', (req, res) => {
    if (songs.length > 0) {
        currentIndex = (currentIndex + 1) % songs.length;
        res.json({ success: true, song: songs[currentIndex].name });
    }
});

app.get('/api/previous', (req, res) => {
    if (songs.length > 0) {
        currentIndex = (currentIndex - 1 + songs.length) % songs.length;
        res.json({ success: true, song: songs[currentIndex].name });
    }
});

app.get('/api/progress', (req, res) => {
    res.json({
        progress: currentProgress,
        duration: currentSongDuration,
        percentage: currentSongDuration > 0 ? (currentProgress / currentSongDuration) * 100 : 0
    });
});

app.get('/api/nowplaying', (req, res) => {
    const currentSong = songs[currentIndex];
    res.json({
        song: currentSong ? currentSong.name : '',
        isPlaying: isPlaying
    });
});

// Mood-based recommendations
app.get('/api/mood', (req, res) => {
    const mood = req.query.mood;
    // Simple mood mapping based on song names
    const moodSongs = songs.filter(song => {
        const name = song.name.toLowerCase();
        switch(mood) {
            case 'happy':
                return name.includes('happy') || name.includes('joy') || name.includes('smile');
            case 'energetic':
                return name.includes('energy') || name.includes('power') || name.includes('strong');
            case 'emotional':
                return name.includes('love') || name.includes('heart') || name.includes('cry');
            case 'chill':
                return name.includes('chill') || name.includes('calm') || name.includes('peace');
            default:
                return true;
        }
    });
    
    res.json({ songs: moodSongs.slice(0, 10) });
});

// Genre-based recommendations  
app.get('/api/genre', (req, res) => {
    const genre = req.query.genre;
    // Simple genre mapping
    const genreSongs = songs.filter(song => {
        const name = song.name.toLowerCase();
        switch(genre) {
            case 'pop':
                return name.includes('pop') || name.includes('love');
            case 'rock':
                return name.includes('rock') || name.includes('metal');
            case 'electronic':
                return name.includes('electronic') || name.includes('dance');
            default:
                return true;
        }
    });
    
    res.json({ songs: genreSongs.slice(0, 10) });
});

// Search songs
app.get('/api/search', (req, res) => {
    const query = req.query.q.toLowerCase();
    const results = songs.filter(song => 
        song.name.toLowerCase().includes(query)
    );
    res.json({ songs: results });
});

// Serve audio files
app.get('/api/audio/:filename', (req, res) => {
    const filename = req.params.filename;
    const filePath = path.join(__dirname, filename);
    
    if (fs.existsSync(filePath)) {
        const stat = fs.statSync(filePath);
        const fileSize = stat.size;
        const range = req.headers.range;
        
        if (range) {
            const parts = range.replace(/bytes=/, "").split("-");
            const start = parseInt(parts[0], 10);
            const end = parts[1] ? parseInt(parts[1], 10) : fileSize - 1;
            const chunksize = (end - start) + 1;
            const file = fs.createReadStream(filePath, { start, end });
            const head = {
                'Content-Range': `bytes ${start}-${end}/${fileSize}`,
                'Accept-Ranges': 'bytes',
                'Content-Length': chunksize,
                'Content-Type': 'audio/mpeg',
            };
            res.writeHead(206, head);
            file.pipe(res);
        } else {
            const head = {
                'Content-Length': fileSize,
                'Content-Type': 'audio/mpeg',
            };
            res.writeHead(200, head);
            fs.createReadStream(filePath).pipe(res);
        }
    } else {
        res.status(404).json({ error: 'File not found' });
    }
});

// Update progress simulation (in a real app, this would come from the audio player)
setInterval(() => {
    if (isPlaying && currentSongDuration > 0) {
        currentProgress += 1;
        if (currentProgress >= currentSongDuration) {
            // Auto-play next song
            currentIndex = (currentIndex + 1) % songs.length;
            currentProgress = 0;
            if (songs[currentIndex]) {
                // Get duration of next song (simplified)
                currentSongDuration = 180; // Default 3 minutes
            }
        }
    }
}, 1000);

// Get duration for a song (simplified)
function getSongDuration(filePath) {
    // Return default duration if can't read
    return 180; // 3 minutes default
}

// Start server
app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
    console.log(`Found ${songs.length} songs in ${__dirname}`);
    songs.forEach(song => console.log(` - ${song.name}`));
});