const API_BASE = 'http://localhost:8080/api';

document.addEventListener('DOMContentLoaded', () => {
    loadPlaylist();
    loadUserPlaylist();
    loadQuickPlay();
    setupEventListeners();
    setupNavigation();
    setupMoodGrid();
    setupGenreGrid();
    updateStats();
    
    setInterval(updateProgress, 1000);
    setInterval(updateNowPlaying, 2000);
});

function setupEventListeners() {
    document.getElementById('prevBtn').addEventListener('click', previousSong);
    document.getElementById('playPauseBtn').addEventListener('click', togglePlayPause);
    document.getElementById('nextBtn').addEventListener('click', nextSong);
    document.getElementById('progressBar').addEventListener('click', seekTo);
    document.getElementById('searchInput').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') searchSongs();
    });
}

function setupNavigation() {
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(item => {
        item.addEventListener('click', (e) => {
            e.preventDefault();
            const page = item.dataset.page;
            navItems.forEach(nav => nav.classList.remove('active'));
            item.classList.add('active');
            document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
            document.getElementById(`${page}Page`).classList.add('active');
            if (page === 'playlist') loadPlaylist();
            if (page === 'userplaylist') loadUserPlaylist();
        });
    });
}

function navigateTo(page) {
    document.querySelector(`.nav-item[data-page="${page}"]`).click();
}

async function updateStats() {
    try {
        const response = await fetch(`${API_BASE}/playlist`);
        const data = await response.json();
        document.getElementById('totalSongs').textContent = data.songs.length;
        
        const userResponse = await fetch(`${API_BASE}/userplaylist`);
        const userData = await userResponse.json();
        document.getElementById('userSongs').textContent = userData.songs.length;
    } catch (error) {
        console.error('Error:', error);
    }
}

async function loadPlaylist() {
    try {
        const response = await fetch(`${API_BASE}/playlist`);
        const data = await response.json();
        displayPlaylist(data.songs, 'playlistContainer', true);
    } catch (error) {
        console.error('Error:', error);
    }
}

async function loadUserPlaylist() {
    try {
        const response = await fetch(`${API_BASE}/userplaylist`);
        const data = await response.json();
        displayPlaylist(data.songs, 'userPlaylistContainer', false);
    } catch (error) {
        console.error('Error:', error);
    }
}

function displayPlaylist(songs, containerId, showAddButton) {
    const container = document.getElementById(containerId);
    if (!container) return;
    
    if (songs.length === 0) {
        container.innerHTML = '<div style="text-align: center; padding: 40px;">No songs yet.</div>';
        return;
    }
    
    container.innerHTML = songs.map(song => `
        <div class="song-card" onclick="playSong('${song.file}')">
            <div class="song-icon">🎵</div>
            <div class="song-name">${song.name}</div>
            ${showAddButton ? `<div class="add-to-playlist" onclick="event.stopPropagation(); addToUserPlaylist('${song.file}')">+ Add to My Playlist</div>` : ''}
        </div>
    `).join('');
}

async function loadQuickPlay() {
    try {
        const response = await fetch(`${API_BASE}/playlist`);
        const data = await response.json();
        const quickSongs = data.songs.slice(0, 4);
        document.getElementById('quickPlayGrid').innerHTML = quickSongs.map(song => `
            <div class="song-card" onclick="playSong('${song.file}')">
                <div class="song-icon">🎵</div>
                <div class="song-name">${song.name}</div>
            </div>
        `).join('');
    } catch (error) {
        console.error('Error:', error);
    }
}

async function playSong(songFile) {
    try {
        await fetch(`${API_BASE}/play?file=${encodeURIComponent(songFile)}`);
        document.getElementById('currentSongTitle').textContent = songFile.replace('.mp3', '');
        document.getElementById('smallSongTitle').textContent = songFile.replace('.mp3', '');
    } catch (error) {
        console.error('Error:', error);
    }
}

async function addToUserPlaylist(songFile) {
    try {
        await fetch(`${API_BASE}/addtoplaylist?file=${encodeURIComponent(songFile)}`);
        alert('Song added to your playlist!');
        loadUserPlaylist();
        updateStats();
    } catch (error) {
        console.error('Error:', error);
    }
}

async function togglePlayPause() {
    try {
        const response = await fetch(`${API_BASE}/nowplaying`);
        const data = await response.json();
        
        if (data.isPlaying) {
            await fetch(`${API_BASE}/pause`);
        } else {
            await fetch(`${API_BASE}/resume`);
        }
    } catch (error) {
        console.error('Error:', error);
    }
}

async function nextSong() {
    try {
        await fetch(`${API_BASE}/next`);
        await updateNowPlaying();
    } catch (error) {
        console.error('Error:', error);
    }
}

async function previousSong() {
    try {
        await fetch(`${API_BASE}/previous`);
        await updateNowPlaying();
    } catch (error) {
        console.error('Error:', error);
    }
}

async function updateProgress() {
    try {
        const response = await fetch(`${API_BASE}/progress`);
        const data = await response.json();
        document.getElementById('progressFill').style.width = `${data.percentage}%`;
        document.getElementById('currentTime').textContent = formatTime(data.progress);
        document.getElementById('totalTime').textContent = formatTime(data.duration);
    } catch (error) {
        console.error('Error:', error);
    }
}

async function updateNowPlaying() {
    try {
        const response = await fetch(`${API_BASE}/nowplaying`);
        const data = await response.json();
        if (data.song && data.song !== '') {
            document.getElementById('currentSongTitle').textContent = data.song;
            document.getElementById('smallSongTitle').textContent = data.song;
            
            const playIcon = document.getElementById('playIcon');
            const pauseIcon = document.getElementById('pauseIcon');
            if (data.isPlaying) {
                playIcon.style.display = 'none';
                pauseIcon.style.display = 'inline';
            } else {
                playIcon.style.display = 'inline';
                pauseIcon.style.display = 'none';
            }
        }
    } catch (error) {
        console.error('Error:', error);
    }
}

function seekTo(e) {
    console.log('Seek feature - backend needs implementation');
}

function formatTime(seconds) {
    if (!seconds || seconds <= 0) return '0:00';
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
}

function setupMoodGrid() {
    const moods = [
        { name: 'Happy', emoji: '😊', mood: 'happy' },
        { name: 'Energetic', emoji: '⚡', mood: 'energetic' },
        { name: 'Emotional', emoji: '😢', mood: 'emotional' },
        { name: 'Lonely', emoji: '😔', mood: 'lonely' },
        { name: 'Party', emoji: '🎉', mood: 'party' },
        { name: 'Chill', emoji: '😌', mood: 'chill' },
        { name: 'Dreamy', emoji: '✨', mood: 'dreamy' },
        { name: 'Rebellious', emoji: '🤘', mood: 'rebellious' },
        { name: 'Groovy', emoji: '🕺', mood: 'groovy' }
    ];
    
    document.getElementById('moodGrid').innerHTML = moods.map(mood => `
        <div class="mood-card" onclick="getMoodRecommendations('${mood.mood}')">
            <div class="mood-emoji">${mood.emoji}</div>
            <div class="mood-name">${mood.name}</div>
        </div>
    `).join('');
}

async function getMoodRecommendations(mood) {
    try {
        const response = await fetch(`${API_BASE}/mood?mood=${mood}`);
        const data = await response.json();
        const resultsDiv = document.getElementById('moodResults');
        
        if (data.songs && data.songs.length > 0) {
            resultsDiv.innerHTML = `
                <h3>🎵 Recommended for you (${mood})</h3>
                <div class="songs-grid">
                    ${data.songs.map(song => `
                        <div class="song-card" onclick="playSong('${song.file}')">
                            <div class="song-icon">🎵</div>
                            <div class="song-name">${song.name}</div>
                            <div class="add-to-playlist" onclick="event.stopPropagation(); addToUserPlaylist('${song.file}')">+ Add to My Playlist</div>
                        </div>
                    `).join('')}
                </div>
            `;
        } else {
            resultsDiv.innerHTML = '<p style="text-align: center; padding: 40px;">No songs found for this mood.</p>';
        }
    } catch (error) {
        console.error('Error:', error);
    }
}

function setupGenreGrid() {
    const genres = [
        { name: 'Pop', emoji: '🎤', genre: 'pop' },
        { name: 'Indie', emoji: '🎸', genre: 'indie' },
        { name: 'Electronic', emoji: '🎧', genre: 'electronic' },
        { name: 'Hip Hop', emoji: '🎙️', genre: 'hiphop' }
    ];
    
    document.getElementById('genreGrid').innerHTML = genres.map(genre => `
        <div class="genre-card" onclick="getGenreRecommendations('${genre.genre}')">
            <div class="genre-emoji">${genre.emoji}</div>
            <div class="genre-name">${genre.name}</div>
        </div>
    `).join('');
}

async function getGenreRecommendations(genre) {
    try {
        const response = await fetch(`${API_BASE}/genre?genre=${genre}`);
        const data = await response.json();
        const resultsDiv = document.getElementById('genreResults');
        
        if (data.songs && data.songs.length > 0) {
            resultsDiv.innerHTML = `
                <h3>🎸 ${genre.toUpperCase()} Recommendations</h3>
                <div class="songs-grid">
                    ${data.songs.map(song => `
                        <div class="song-card" onclick="playSong('${song.file}')">
                            <div class="song-icon">🎵</div>
                            <div class="song-name">${song.name}</div>
                            <div class="add-to-playlist" onclick="event.stopPropagation(); addToUserPlaylist('${song.file}')">+ Add to My Playlist</div>
                        </div>
                    `).join('')}
                </div>
            `;
        } else {
            resultsDiv.innerHTML = '<p style="text-align: center; padding: 40px;">No songs found for this genre.</p>';
        }
    } catch (error) {
        console.error('Error:', error);
    }
}

async function searchSongs() {
    const query = document.getElementById('searchInput').value;
    if (!query.trim()) return;
    
    try {
        const response = await fetch(`${API_BASE}/search?q=${encodeURIComponent(query)}`);
        const data = await response.json();
        const resultsDiv = document.getElementById('searchResults');
        
        if (data.songs && data.songs.length > 0) {
            resultsDiv.innerHTML = `
                <h3>Search Results for "${query}"</h3>
                <div class="songs-grid">
                    ${data.songs.map(song => `
                        <div class="song-card" onclick="playSong('${song.file}')">
                            <div class="song-icon">🎵</div>
                            <div class="song-name">${song.name}</div>
                            <div class="add-to-playlist" onclick="event.stopPropagation(); addToUserPlaylist('${song.file}')">+ Add to My Playlist</div>
                        </div>
                    `).join('')}
                </div>
            `;
        } else {
            resultsDiv.innerHTML = '<p style="text-align: center; padding: 40px;">No songs found.</p>';
        }
    } catch (error) {
        console.error('Error:', error);
    }
}