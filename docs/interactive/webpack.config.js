const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const MiniCssExtractPlugin = require('mini-css-extract-plugin');
const CopyWebpackPlugin = require('copy-webpack-plugin');

module.exports = (env, argv) => {
  const isDevelopment = argv.mode === 'development';
  
  return {
    entry: './src/index.js',
    output: {
      path: path.resolve(__dirname, 'dist'),
      filename: isDevelopment ? '[name].js' : '[name].[contenthash].js',
      clean: true,
      publicPath: '/'
    },
    
    devtool: isDevelopment ? 'source-map' : false,
    
    devServer: {
      static: {
        directory: path.join(__dirname, 'dist'),
      },
      port: 3000,
      hot: true,
      historyApiFallback: true,
      proxy: {
        '/api': {
          target: 'http://localhost:8080',
          changeOrigin: true,
          pathRewrite: {
            '^/api': ''
          }
        }
      }
    },
    
    module: {
      rules: [
        {
          test: /\.js$/,
          exclude: /node_modules/,
          use: {
            loader: 'babel-loader',
            options: {
              presets: ['@babel/preset-env']
            }
          }
        },
        {
          test: /\.css$/,
          use: [
            isDevelopment ? 'style-loader' : MiniCssExtractPlugin.loader,
            'css-loader'
          ]
        },
        {
          test: /\.(png|svg|jpg|jpeg|gif)$/i,
          type: 'asset/resource',
          generator: {
            filename: 'assets/images/[name].[hash][ext]'
          }
        },
        {
          test: /\.(woff|woff2|eot|ttf|otf)$/i,
          type: 'asset/resource',
          generator: {
            filename: 'assets/fonts/[name].[hash][ext]'
          }
        },
        {
          test: /\.(md|markdown)$/i,
          type: 'asset/source'
        },
        {
          test: /\.json$/i,
          type: 'asset/resource',
          generator: {
            filename: 'assets/data/[name].[hash][ext]'
          }
        }
      ]
    },
    
    plugins: [
      new HtmlWebpackPlugin({
        template: './src/index.html',
        title: 'ECScope Interactive Documentation',
        meta: {
          'viewport': 'width=device-width, initial-scale=1',
          'description': 'Interactive documentation for ECScope Educational ECS Engine',
          'keywords': 'ECS, game engine, C++, education, performance, documentation'
        }
      }),
      
      ...(isDevelopment ? [] : [
        new MiniCssExtractPlugin({
          filename: 'assets/css/[name].[contenthash].css'
        })
      ]),
      
      new CopyWebpackPlugin({
        patterns: [
          {
            from: 'src/assets',
            to: 'assets',
            noErrorOnMissing: true
          },
          {
            from: 'src/content',
            to: 'content',
            noErrorOnMissing: true
          },
          {
            from: '../generated', // API documentation
            to: 'api',
            noErrorOnMissing: true
          },
          {
            from: '../../examples', // Code examples
            to: 'examples',
            noErrorOnMissing: true,
            globOptions: {
              ignore: ['**/build/**', '**/CMakeLists.txt']
            }
          }
        ]
      })
    ],
    
    resolve: {
      alias: {
        '@': path.resolve(__dirname, 'src'),
        '@content': path.resolve(__dirname, 'src/content'),
        '@assets': path.resolve(__dirname, 'src/assets'),
        '@components': path.resolve(__dirname, 'src/components'),
        '@utils': path.resolve(__dirname, 'src/utils')
      }
    },
    
    optimization: {
      splitChunks: {
        chunks: 'all',
        cacheGroups: {
          vendor: {
            test: /[\\/]node_modules[\\/]/,
            name: 'vendors',
            chunks: 'all'
          },
          monaco: {
            test: /[\\/]node_modules[\\/]monaco-editor[\\/]/,
            name: 'monaco',
            chunks: 'all'
          }
        }
      }
    }
  };
};