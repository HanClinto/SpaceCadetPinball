// A shared high score table for the web version: the game's Module.scoreboard (see the README), talking to a
// scores service such as server.py next to this file. Every finished one-player game is sent to the service; after
// each game, a dialog says what happened ("New personal best! You passed sam!") and cycles through three tables:
// personal bests (everyone once), all-time (every game) and your own games. The game's own high score table (its
// "High Score 1-5" messages) is the all-time top 5. Each version of the table has its own board.
//
// Load it before the game's script:
//   <script src="scoreboard.js" data-api="scores/api" data-scores-page="scores/"></script>
//   <script async src="SpaceCadetPinball.js"></script>
// data-api        the service's API, relative to the page (default "scores/api")
// data-boards     the boards for XP and Full Tilt Space Cadet (default "spacecadet,spacecadet-ft")
// data-scores-page  a page listing all the scores, linked from the dialog (optional)
//
// Who's playing: a service behind a sign-in says (its "me"); otherwise the player is asked for a name once, which
// this browser remembers and sends with each request (X-Player-Name).
(function () {
    'use strict';
    var script = document.currentScript, config = (script && script.dataset) || {};
    var base = new URL((config.api || 'scores/api').replace(/\/?$/, '/'), location.href).href;
    var boards = (config.boards || 'spacecadet,spacecadet-ft').split(',');
    var scoresPage = config.scoresPage || null;
    var VIEWS = [['best', 'Personal bests'], ['all', 'All-time'], ['mine', 'Your games']];
    var ROTATE_MS = 6000;
    var board = null, data = null, gameReady = false, dialog = null, rotateTimer = null, viewIndex = 0, highlight = null;

    function store(key, value) {
        try {
            if (value === undefined) return JSON.parse(localStorage.getItem('scoreboard.' + key) || 'null');
            localStorage.setItem('scoreboard.' + key, JSON.stringify(value));
        } catch (e) { return null; }
    }

    function request(method, path, body) {
        var headers = body ? { 'Content-Type': 'application/json' } : {};
        if (store('name')) headers['X-Player-Name'] = encodeURIComponent(store('name'));
        return fetch(base + path, {
            method: method, credentials: 'same-origin', headers: headers,
            body: body ? JSON.stringify(body) : undefined
        }).then(function (r) {
            if (!r.ok) throw new Error('the scores service answered ' + r.status);
            return r.json();
        });
    }

    // The game's own table (its "High Score 1-5" messages, and "HIGH SCORE" when a game makes it): the all-time top 5
    function pushTable() {
        if (!gameReady || !data) return;
        for (var i = 0; i < 5; i++) {
            var e = data.views.all[i];
            Module.ccall('web_set_high_score', null, ['number', 'string', 'number'],
                [i, e ? e.user : '', e ? e.value : -999]);
        }
    }

    function load() {
        if (!board) return Promise.resolve();
        return request('GET', 'boards/' + board + '?limit=10').then(function (d) {
            data = d;
            pushTable();
            if (dialog) render();
            return d;
        }).catch(function (e) { console.warn('High scores:', e.message); });
    }

    // Games that couldn't be sent (offline, the service restarting) wait in this browser and go on the next visit.
    // Each has an id, so a game sent twice counts once.
    function send(game) {
        return request('POST', 'boards/' + game.board + '/entries', { value: game.value, game_id: game.game_id })
            .then(function (result) {
                store('pending', (store('pending') || []).filter(function (p) { return p.game_id !== game.game_id; }));
                if (game.board === board) {
                    data = result;
                    pushTable();
                }
                return result;
            });
    }
    function sendPending() {
        (store('pending') || []).forEach(function (game) { send(game).catch(function () { }); });
    }

    // A service without a sign-in identifies players by the name they give
    function needName() {
        return data && !data.me && !store('name');
    }
    function askName() {
        var name = (prompt('Your name for the high scores:', store('name') || '') || '').trim().slice(0, 32);
        if (name) store('name', name);
        return name;
    }

    function gameId() {
        return (crypto.randomUUID ? crypto.randomUUID() : Date.now() + '-' + Math.random().toString(36).slice(2));
    }

    var fmt = function (n) { return n === null || n === undefined ? '–' : n.toLocaleString('en-US'); };
    function ago(at) {
        var s = Math.max(0, Date.now() / 1000 - at);
        if (s < 60) return 'just now';
        var units = [['day', 86400], ['hour', 3600], ['minute', 60]];
        for (var i = 0; i < units.length; i++)
            if (s >= units[i][1]) { var k = Math.floor(s / units[i][1]); return k + ' ' + units[i][0] + (k > 1 ? 's' : '') + ' ago'; }
    }
    function esc(s) {
        return String(s).replace(/[&<>"]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]; });
    }

    var css = '' +
        '#sb-scores{position:fixed;inset:0;z-index:20;display:flex;align-items:center;justify-content:center;background:rgba(0,0,0,.55);font:15px/1.4 system-ui,-apple-system,"Segoe UI",sans-serif;color:#e8e8ee}' +
        '#sb-scores .box{width:min(400px,calc(100vw - 24px));max-height:calc(100dvh - 24px);overflow:auto;background:#16161c;border:1px solid #33333f;border-radius:14px;padding:16px;box-shadow:0 10px 40px rgba(0,0,0,.6)}' +
        '#sb-scores h2{margin:0 0 4px;font-size:18px}' +
        '#sb-scores .news{color:#ffd166;font-weight:600;margin:6px 0 2px;min-height:1.4em}' +
        '#sb-scores .sub{color:#9a9aa8;font-size:13px}' +
        '#sb-scores .tabs{display:flex;gap:6px;margin:12px 0 8px}' +
        '#sb-scores .tabs button{flex:1;font:inherit;font-size:13px;color:#e8e8ee;background:#2a2a35;border:0;border-radius:999px;padding:5px 0;cursor:pointer}' +
        '#sb-scores .tabs button.on{background:#5b8cff;color:#fff}' +
        '#sb-scores table{width:100%;border-collapse:collapse;font-size:15px}' +
        '#sb-scores td{padding:5px 4px;border-bottom:1px solid #26262f}' +
        '#sb-scores td.r{width:1.8em;color:#9a9aa8}#sb-scores td.n{text-align:right;font-variant-numeric:tabular-nums}' +
        '#sb-scores tr.me td{background:#2c2a1a}#sb-scores tr.new td{background:#3a3317;font-weight:600}' +
        '#sb-scores .empty{color:#9a9aa8;padding:10px 4px}' +
        '#sb-scores .foot{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-top:12px;font-size:13px;color:#9a9aa8}' +
        '#sb-scores .foot a{color:#8fb0ff}' +
        '#sb-scores .ok{font:inherit;font-weight:600;color:#fff;background:#5b8cff;border:0;border-radius:10px;padding:8px 22px;cursor:pointer}' +
        '#sb-toast{position:fixed;left:50%;top:16px;transform:translateX(-50%);z-index:21;background:#16161c;color:#e8e8ee;border:1px solid #33333f;border-radius:10px;padding:8px 14px;font:14px system-ui,sans-serif;max-width:calc(100vw - 32px)}';
    var style = document.createElement('style');
    style.textContent = css;
    document.head.appendChild(style);

    function toast(text) {
        var t = document.getElementById('sb-toast') || document.body.appendChild(Object.assign(document.createElement('div'), { id: 'sb-toast' }));
        t.textContent = text;
        clearTimeout(toast.timer);
        toast.timer = setTimeout(function () { t.remove(); }, 5000);
    }

    // What this game did, in a line: a new best, who it passed, where it stands
    function news(result) {
        if (!result || !result.entry) return '';
        var parts = [];
        if (result.previous_best === null) parts.push('Your first score!');
        else if (result.new_best) parts.push('New personal best!');
        if (result.passed && result.passed.length)
            parts.push('You passed ' + result.passed.slice(0, 3).join(', ') + (result.passed.length > 3 ? ' and ' + (result.passed.length - 3) + ' more' : '') + '!');
        if (result.entry.rank <= 10) parts.push('#' + result.entry.rank + ' all-time.');
        return parts.join(' ') || fmt(result.entry.value) + ' points.';
    }

    function render() {
        if (!dialog) return;
        var view = VIEWS[viewIndex][0];
        var rows = data ? data.views[view] : [];
        var me = data && (data.me || store('name'));
        var html = '<div class="box" role="dialog" aria-label="High scores"><h2>🏆 ' + esc(data ? data.board.title : 'High scores') + '</h2>';
        html += '<div class="news">' + esc(news(highlight)) + '</div>';
        if (data) {
            html += '<div class="sub">' + (data.my_best !== null
                ? 'Your best: ' + fmt(data.my_best) + ' · #' + data.my_rank + ' of ' + data.players + ' player' + (data.players > 1 ? 's' : '')
                : 'No games yet: launch the ball!') + '</div>';
        } else {
            html += '<div class="sub">Loading...</div>';
        }
        html += '<div class="tabs">' + VIEWS.map(function (v, i) {
            return '<button type="button" data-view="' + i + '" class="' + (i === viewIndex ? 'on' : '') + '">' + v[1] + '</button>';
        }).join('') + '</div>';
        if (rows && rows.length) {
            html += '<table>' + rows.map(function (e, i) {
                var cls = highlight && highlight.entry && e.id === highlight.entry.id ? 'new' : (e.user === me && view !== 'mine' ? 'me' : '');
                return '<tr class="' + cls + '"><td class="r">' + (i + 1) + '</td><td>' +
                    (view === 'mine' ? '<span class="sub">' + ago(e.at) + '</span>' : esc(e.user)) +
                    '</td><td class="n">' + fmt(e.value) + '</td></tr>';
            }).join('') + '</table>';
        } else if (data) {
            html += '<div class="empty">No games yet</div>';
        }
        var who = me ? 'Playing as ' + esc(me) + (data && !data.me ? ' (<a href="#" data-rename>change</a>)' : '') : '';
        var links = [who, scoresPage ? '<a href="' + esc(scoresPage) + '" target="_blank">All scores</a>' : ''].filter(Boolean);
        html += '<div class="foot"><span>' + links.join(' · ') + '</span><button type="button" class="ok">OK</button></div></div>';
        dialog.innerHTML = html;
    }

    function rotate() {
        clearInterval(rotateTimer);
        rotateTimer = setInterval(function () { viewIndex = (viewIndex + 1) % VIEWS.length; render(); }, ROTATE_MS);
    }

    // Keys go to the dialog, not the game, while it's open
    function onKey(e) {
        if (!dialog) return;
        e.stopImmediatePropagation();
        if (e.type !== 'keydown') return;
        e.preventDefault();
        if (e.key === 'Enter' || e.key === 'Escape' || e.key === ' ') close();
        else if (e.key === 'ArrowRight' || e.key === 'ArrowLeft') {
            viewIndex = (viewIndex + (e.key === 'ArrowRight' ? 1 : VIEWS.length - 1)) % VIEWS.length;
            clearInterval(rotateTimer);
            render();
        }
    }
    window.addEventListener('keydown', onKey, true);
    window.addEventListener('keyup', onKey, true);

    function open(result) {
        highlight = result || null;
        viewIndex = 0;
        if (!dialog) {
            dialog = document.createElement('div');
            dialog.id = 'sb-scores';
            dialog.addEventListener('click', function (e) {
                var v = e.target.getAttribute('data-view');
                if (v !== null) { viewIndex = +v; clearInterval(rotateTimer); render(); }
                else if (e.target.hasAttribute('data-rename')) { e.preventDefault(); if (askName()) load(); }
                else if (e.target === dialog || e.target.classList.contains('ok')) close();
            });
            document.body.appendChild(dialog);
        }
        render();
        rotate();
        if (!result) load();
    }

    function close() {
        clearInterval(rotateTimer);
        if (dialog) dialog.remove();
        dialog = null;
        highlight = null;
    }

    window.Module = window.Module || {};
    Module.scoreboard = {
        onTableNeeded: function () {
            gameReady = true;
            // Which version of the table the game loaded: web_game_state says Full Tilt with 8
            var next = boards[Module.ccall('web_game_state', 'number', [], []) & 8 ? 1 : 0];
            if (next !== board) {
                board = next;
                data = null;
                load().then(sendPending);
            }
            pushTable();
        },
        submit: function (game) {
            var score = game.scores[0];
            if (game.players !== 1) return toast('High scores count one-player games only.');
            if (game.cheated) return toast('Cheats were on, so this game isn’t on the high scores.');
            if (!score) return;
            if (needName() && !askName()) return toast('No name, so this game isn’t on the high scores.');
            var entry = { board: board, value: score, game_id: gameId() };
            store('pending', (store('pending') || []).concat([entry]));
            send(entry).then(open).catch(function (e) {
                toast('Couldn’t save your score yet (' + e.message + '); it’ll be sent on your next visit.');
            });
        },
        show: function () { open(null); }
    };

    setInterval(load, 120000);
    document.addEventListener('visibilitychange', function () { if (!document.hidden) load(); });
})();
