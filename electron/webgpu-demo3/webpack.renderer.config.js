const rules = require('./webpack.rules');
const CopyWebpackPlugin = require('copy-webpack-plugin');
const path = require('path');

module.exports = {
  // Put your normal webpack config below here
  module: {
    rules,
  },
  plugins: [
    new CopyWebpackPlugin({
      patterns: [
        { from: path.resolve(__dirname, 'src/client.js'), to: 'main_window/client.js' },
        { from: path.resolve(__dirname, 'src/client.wasm'), to: 'main_window/client.wasm' },
        { from: path.resolve(__dirname, 'src/client.data'), to: 'main_window/client.data' },
      ],
    }),
  ],
};
