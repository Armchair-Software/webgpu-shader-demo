const rules = require('./webpack.rules');
const CopyWebpackPlugin = require('copy-webpack-plugin');
const path = require('path');

rules.push({
  test: /\.css$/,
  use: [{ loader: 'style-loader' }, { loader: 'css-loader' }],
});

module.exports = {
  // Put your normal webpack config below here
  module: {
    rules,
  },
  plugins: [
    new CopyWebpackPlugin({
      patterns: [
        { from: path.resolve(__dirname, 'src/client.js'), to: 'client.js' },
        { from: path.resolve(__dirname, 'src/client.wasm'), to: 'client.wasm' },
        { from: path.resolve(__dirname, 'src/client.data'), to: 'client.data' },
      ],
    }),
  ],
};
