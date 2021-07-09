const EventEmitter = require('events').EventEmitter;
const GameNetworkingSockets = require('./build/Debug/steamworks.node');
const inherits = require('util').inherits;

inherits(GameNetworkingSockets.GameNetworkingServer, EventEmitter);

const PORT = 3756;

GameNetworkingSockets.GameNetworkingServer.init(PORT);

    let instance = new GameNetworkingSockets.GameNetworkingServer({
        port: PORT
    });

    instance.on('start', () => {
    console.log('started serv');
    });

    instance.on('connected', (event) => {
    let client = new this.clientClass(event.handle, event.connectionDescription, event.addRemoteIPv4, event.addrRemotePort, event.identityRemoteSteamID64);

    this.clients[client.handle] = client;

    this.emit('connected', client);
    });

    instance.on('disconnected', (event) => {
    let client = this.clients[event.handle];
    console.log(event);
    console.log(client);

    client.emit('disconnected');

    this.emit('disconnected', event);

    delete this.clients[client.handle];
    });

    instance.on('message', (event) => {
    let client = this.clients[event.handle];

    console.log(event);

    client.emit(event);

    this.emit('message', event);
    });

    instance.on('end', () => {
    this.emit('closed');
    });

    instance.startServer();

    while(true) {
        instance.runCallbacks();
        instance.pollIncomingMessages();
    }

//module.exports.SteamGameServer = SteamGameServer;
//module.exports.SteamGameClient = SteamGameClient;
