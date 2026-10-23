#!/usr/bin/env python3
"""A sample shared high score service for the web version, with nothing to install (Python 3.8+ and its SQLite).

    python3 Platform/Web/scoreboard/server.py bin            # serves the built game and its scores
    open http://localhost:8000/

It serves the folder given (the build's bin/), adds scoreboard.js to the game's page, and keeps every game in
scores.db (next to this file, or --db). Every game is kept with its time, so each table is a query: all-time (each
player's top 5 games, so one player can't fill it), personal bests, one player's games.

Who's playing: behind a reverse proxy that signs people in, pass --user-header with the header it sets (for
example Remote-User) and only that is trusted. Otherwise players type a name once (the page remembers it), which
is the honor system: anyone can post any score under any name. Fine among friends; don't put it on the internet
as is.

API (also what scoreboard.js expects from any service):
    GET  scores/api/boards/<board>?limit=10   the board: {board, me, views: {all, best, mine}, my_best, my_rank,
                                              games, players}
    POST scores/api/boards/<board>/entries    {"value": 123, "game_id": "..."}: the same as GET, plus {entry,
                                              previous_best, new_best, passed}
    GET  scores/api/recent                    the latest games, every board
"""
import argparse
import json
import mimetypes
import os
import re
import sqlite3
import threading
import time
import urllib.parse
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

HERE = os.path.dirname(os.path.abspath(__file__))
MAX_VALUE = 2**31 - 1  # the game keeps scores in 32-bit integers
PER_PLAYER = 5  # games each player can have on the all-time table
TITLES = {"spacecadet": "XP Space Cadet", "spacecadet-ft": "Full Tilt Space Cadet"}
BOARD = re.compile(r"^[a-z0-9-]{1,40}$")
SCHEMA = """
CREATE TABLE IF NOT EXISTS entries (
    id INTEGER PRIMARY KEY,
    board TEXT NOT NULL,
    user TEXT NOT NULL,
    value INTEGER NOT NULL,
    at INTEGER NOT NULL,
    game_id TEXT
);
CREATE INDEX IF NOT EXISTS entries_board_value ON entries (board, value DESC, at);
CREATE UNIQUE INDEX IF NOT EXISTS entries_game ON entries (board, user, game_id) WHERE game_id IS NOT NULL;
"""
lock = threading.Lock()


def query(db, sql, args=()):
    return [dict(r) for r in db.execute(sql, args)]


def top_games(per_player):
    """Each player's best games on a board (up to per_player of them), best first, as SQL taking the board"""
    return f"""SELECT id, user, value, at FROM (
                SELECT *, ROW_NUMBER() OVER (PARTITION BY user ORDER BY value DESC, at) AS n FROM entries WHERE board = ?)
              WHERE n <= {int(per_player)} ORDER BY value DESC, at"""


def summary(db, board, user, limit):
    """A board's three tables (best first; ties go to whoever got there first), and where the player stands."""
    everyone = query(db, top_games(1), (board,))
    mine = [i for i, e in enumerate(everyone) if e["user"] == user]
    totals = db.execute("SELECT COUNT(*), COUNT(DISTINCT user) FROM entries WHERE board = ?", (board,)).fetchone()
    return {
        "board": {"id": board, "title": TITLES.get(board, board)},
        "me": user if Handler.user_header else None,  # None: the page gives the name
        "views": {
            "all": query(db, top_games(PER_PLAYER) + " LIMIT ?", (board, limit)),
            "best": everyone[:limit],
            "mine": query(db, "SELECT id, user, value, at FROM entries WHERE board = ? AND user = ? "
                              "ORDER BY value DESC, at LIMIT ?", (board, user, limit)),
        },
        "my_best": everyone[mine[0]]["value"] if mine else None,
        "my_rank": mine[0] + 1 if mine else None,
        "games": totals[0],
        "players": totals[1],
    }


class Handler(SimpleHTTPRequestHandler):
    db_path = None
    user_header = None

    def player(self):
        if self.user_header:
            return self.headers.get(self.user_header, "")
        return urllib.parse.unquote(self.headers.get("X-Player-Name", "")).strip()[:32]

    def reply(self, code, body):
        data = json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def api(self, method):
        url = urllib.parse.urlsplit(self.path)
        parts = url.path.split("/")[3:]  # after /scores/api/
        user = self.player()
        limit = max(1, min(int((urllib.parse.parse_qs(url.query).get("limit") or ["10"])[0]), 100))
        with lock, sqlite3.connect(self.db_path) as db:
            db.row_factory = sqlite3.Row
            if method == "GET" and parts == ["recent"]:
                return self.reply(200, query(db, "SELECT id, board, user, value, at FROM entries "
                                                 "ORDER BY at DESC LIMIT ?", (limit,)))
            if len(parts) < 2 or parts[0] != "boards" or not BOARD.match(parts[1]):
                return self.reply(404, {"error": "no such board"})
            board = parts[1]
            if method == "GET" and len(parts) == 2:
                return self.reply(200, summary(db, board, user, limit))
            if method != "POST" or parts[2:] != ["entries"]:
                return self.reply(404, {"error": "not found"})
            # Only same-site scripts send JSON (a form on another site can't)
            if self.headers.get("Content-Type", "").split(";")[0] != "application/json":
                return self.reply(415, {"error": "send JSON"})
            if not user:
                return self.reply(403, {"error": "who's playing? (a name, or a sign-in in front of this service)"})
            body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", 0))) or b"{}")
            value, game_id = body.get("value"), (str(body.get("game_id"))[:64] if body.get("game_id") else None)
            if not isinstance(value, int) or isinstance(value, bool) or not 0 <= value <= MAX_VALUE:
                return self.reply(400, {"error": "value must be a whole number from 0 to 2147483647"})
            previous = db.execute("SELECT MAX(value) FROM entries WHERE board = ? AND user = ?",
                                  (board, user)).fetchone()[0]
            new_best = previous is None or value > previous
            passed = [r["user"] for r in db.execute(
                "SELECT user, MAX(value) AS best FROM entries WHERE board = ? AND user != ? GROUP BY user "
                "HAVING best < ? AND best >= ? ORDER BY best DESC", (board, user, value, previous or 0))] if new_best else []
            now = int(time.time())
            try:
                entry_id = db.execute("INSERT INTO entries (board, user, value, at, game_id) VALUES (?, ?, ?, ?, ?)",
                                      (board, user, value, now, game_id)).lastrowid
            except sqlite3.IntegrityError:  # the same game sent again
                r = db.execute("SELECT id, value, at FROM entries WHERE board = ? AND user = ? AND game_id = ?",
                               (board, user, game_id)).fetchone()
                entry_id, value, now, passed, new_best = r["id"], r["value"], r["at"], [], False
            # Its place on the all-time table; None when it isn't one of the player's top games
            rank = next((i + 1 for i, e in enumerate(query(db, top_games(PER_PLAYER), (board,)))
                         if e["id"] == entry_id), None)
            result = summary(db, board, user, limit)
            result.update(entry={"id": entry_id, "value": value, "at": now, "rank": rank},
                          previous_best=previous, new_best=new_best, passed=passed)
            return self.reply(200, result)

    def do_GET(self):
        if self.path.startswith("/scores/api/"):
            return self.api("GET")
        page = urllib.parse.urlsplit(self.path).path
        if page in ("/", "/index.html", "/SpaceCadetPinball.html"):
            return self.game_page()
        if page == "/scoreboard.js":
            return self.file(os.path.join(HERE, "scoreboard.js"))
        return super().do_GET()

    def do_POST(self):
        if self.path.startswith("/scores/api/"):
            return self.api("POST")
        self.send_error(404)

    def game_page(self):
        """The built page, with scoreboard.js loaded before the game"""
        name = "index.html" if os.path.exists(os.path.join(self.directory, "index.html")) else "SpaceCadetPinball.html"
        page = open(os.path.join(self.directory, name), encoding="utf-8").read()
        game = re.search(r"<script async src=\"?SpaceCadetPinball\.js\"?></script>", page)
        if game and "scoreboard.js" not in page:
            page = page[:game.start()] + '<script src="scoreboard.js"></script>' + page[game.start():]
        data = page.encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def file(self, path):
        data = open(path, "rb").read()
        self.send_response(200)
        self.send_header("Content-Type", mimetypes.guess_type(path)[0] or "application/octet-stream")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("folder", nargs="?", default="bin", help="the built game (default: bin)")
    ap.add_argument("--port", type=int, default=8000)
    ap.add_argument("--host", default="127.0.0.1", help="0.0.0.0 to let other devices (phones) in")
    ap.add_argument("--db", default=os.path.join(HERE, "scores.db"))
    ap.add_argument("--user-header", help="trust this header for who's playing (set by a sign-in proxy)")
    a = ap.parse_args()
    with sqlite3.connect(a.db) as db:
        db.executescript(SCHEMA)
    Handler.db_path, Handler.user_header = a.db, a.user_header
    mimetypes.add_type("application/wasm", ".wasm")
    handler = lambda *args, **kw: Handler(*args, directory=a.folder, **kw)
    print(f"Space Cadet with shared high scores: http://{a.host}:{a.port}/ (scores in {a.db})")
    ThreadingHTTPServer((a.host, a.port), handler).serve_forever()


if __name__ == "__main__":
    main()
