const EventEmitter = require('events').EventEmitter;
const GameNetworkingSockets = require('./build/Release/steamworks.node');
const inherits = require('util').inherits;

inherits(GameNetworkingSockets.GameNetworkingServer, EventEmitter);
inherits(GameNetworkingSockets.GameNetworkingClient, EventEmitter);

class GameClient extends EventEmitter {
    constructor(server, handle, connectionDescription, ip, port, steamId) {
        super();

        this._server = server;
        this.handle = handle;
        this.connectionDescription = connectionDescription;
        this.ip = ip;
        this.port = port;
        this.steamId = steamId;
    }

    sendMessage(message) {
        this._server.sendMessage(this, message);
    }
}

class SteamGameServer extends EventEmitter {
    constructor(port, clientClass = GameClient) {
        super();

        this.port = port;
        this.clientClass = clientClass;
        this.clients = {};
        this.running = false;

        GameNetworkingSockets.GameNetworkingServer.init(port);
    }

    startServer() {
        if (this.running)
            return;

        this.running = true;

        this.instance = new GameNetworkingSockets.GameNetworkingServer({
            port: this.port
        });

        this.instance.on('start', () => {
            this.emit('started');
        });

        this.instance.on('connected', (event) => {
            let client = new this.clientClass(this, event.handle, event.connectionDescription, event.addRemoteIPv4, event.addrRemotePort, event.identityRemoteSteamID64);
            
            this.clients[client.handle] = client;

            this.emit('connected', client);
        });

        this.instance.on('disconnected', (event) => {
            let client = this.clients[event.handle];

            delete event.handle;
            client.emit('disconnected', event);

            event.client = client;
            this.emit('disconnected', event);

            delete this.clients[client.handle];
        });

        this.instance.on('message', (event) => {
            let client = this.clients[event.handle];

            // event.identityPeerSteamID64;
            // idk maybe check client identity?

            client.emit('message', { data: event.data });

            this.emit('message', {
                client,
                data: event.data
            });
        });

        this.instance.on('end', () => {
            this.emit('closed');
        });

        this.instance.startServer();
    }

    /*stopServer() { whatever this doesn't seem to work fuck it
        this.instance.stopServer();
        this.running = false;
    }*/

    registerServer(serverName, passwordProtected, tags) {
        GameNetworkingSockets.GameNetworkingServer.register(serverName, passwordProtected, tags);
    }

    sendMessage(client, message) {
        this.instance.sendMessage(client.handle, message);
    }

    runCallbacks() {
        this.instance.runCallbacks();
        this.instance.pollIncomingMessages();
    }
}

class SteamGameClient extends EventEmitter {
    constructor(server) {
        super();

        this.server = server;
        this.clients = {};
        this.running = false;

        GameNetworkingSockets.GameNetworkingClient.init();
    }

    startClient() {
        if (this.running)
            return;

        this.running = true;

        this.instance = new GameNetworkingSockets.GameNetworkingClient({
            server: this.server
        });

        this.instance.on('start', () => {
            this.emit('started');
        });

        this.instance.on('connected', (event) => {
            this.emit('connected', event);
        });

        this.instance.on('disconnected', (event) => {
            this.emit('disconnected', event);
        });

        this.instance.on('message', (event) => {
            this.emit('message', event);
        });

        this.instance.on('end', () => {
            this.emit('closed');
        });

        this.instance.startClient();
    }

    getAuthSessionTicket() {
        return GameNetworkingSockets.GameNetworkingClient.getAuthSessionTicket();
    }

    /*stopServer() { whatever this doesn't seem to work fuck it
        this.instance.stopServer();
        this.running = false;
    }*/

    sendMessage(message) {
        this.instance.sendMessage(message);
    }

    runCallbacks() {
        this.instance.runCallbacks();
        this.instance.pollIncomingMessages();
    }
}

inherits(SteamGameServer, EventEmitter);
inherits(GameClient, EventEmitter);
inherits(SteamGameClient, EventEmitter);

module.exports.SteamGameServer = SteamGameServer;
module.exports.GameClient = GameClient;
module.exports.SteamGameClient = SteamGameClient;
