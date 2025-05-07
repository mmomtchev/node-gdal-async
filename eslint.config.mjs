import structuredClone from '@ungap/structured-clone'
if (!('structuredClone' in globalThis)) {
  console.log('Using a JS implementation of structuredClone')
  globalThis.structuredClone = structuredClone
}

import mocha from 'eslint-plugin-mocha'
import preferArrow from 'eslint-plugin-prefer-arrow'
import globals from 'globals'
import typescriptEslint from '@typescript-eslint/eslint-plugin'
import tsParser from '@typescript-eslint/parser'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import js from '@eslint/js'
import { FlatCompat } from '@eslint/eslintrc'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)
const compat = new FlatCompat({
  baseDirectory: __dirname,
  recommendedConfig: js.configs.recommended,
  allConfig: js.configs.all
})

export default [
  ...compat.extends('eslint:recommended', 'plugin:array-func/recommended'),
  {
    plugins: {
      mocha,
      'prefer-arrow': preferArrow
    },

    languageOptions: {
      globals: {
        ...globals.mocha,
        ...globals.node
      }
    },

    rules: {
      'prefer-const': [ 'error', {
        destructuring: 'all',
        ignoreReadBeforeAssign: false
      } ],

      'array-bracket-spacing': [ 2, 'always' ],
      'array-callback-return': 1,
      'arrow-parens': 2,
      'arrow-spacing': 1,
      curly: [ 'error', 'multi-line', 'consistent' ],

      'key-spacing': [ 2, {
        beforeColon: false,
        afterColon: true
      } ],

      'comma-dangle': [ 2, 'never' ],
      'dot-notation': 1,
      'dot-location': [ 2, 'property' ],
      'eol-last': 2,
      'handle-callback-err': 1,

      indent: [ 'error', 2, {
        SwitchCase: 1
      } ],

      'brace-style': 1,
      'keyword-spacing': 1,
      'new-parens': 1,
      'no-await-in-loop': 1,
      'no-self-compare': 1,
      'no-throw-literal': 1,
      'no-console': 0,
      'no-debugger': 0,
      'no-else-return': 1,
      'no-extra-parens': 1,
      'no-floating-decimal': 1,
      'no-implicit-coercion': 0,
      'no-mixed-spaces-and-tabs': 1,
      'no-multi-spaces': 1,
      'no-multiple-empty-lines': 1,
      'no-path-concat': 1,
      'no-return-await': 1,
      'no-trailing-spaces': 1,
      'no-unneeded-ternary': 1,

      'no-unused-vars': [ 2, {
        ignoreRestSiblings: true,
        args: 'after-used',
        argsIgnorePattern: '^_',
        caughtErrorsIgnorePattern: '^_'
      } ],

      'no-useless-concat': 1,
      'no-useless-call': 1,
      'no-useless-constructor': 1,
      'no-useless-return': 1,
      'no-var': 2,
      'no-with': 1,
      'vars-on-top': 1,
      'object-curly-spacing': [ 2, 'always' ],
      'template-curly-spacing': [ 'error', 'never' ],
      'prefer-promise-reject-errors': 1,
      'prefer-template': 1,
      'prefer-arrow-callback': 1,
      'arrow-body-style': [ 'error', 'as-needed' ],

      'prefer-arrow/prefer-arrow-functions': [ 'warn', {
        singleReturnOnly: true,
        classPropertiesAllowed: true
      } ],

      quotes: [ 'error', 'single', {
        avoidEscape: true,
        allowTemplateLiterals: true
      } ],

      'mocha/no-exclusive-tests': 'error',
      'mocha/no-identical-title': 'error',
      'mocha/no-nested-tests': 'error',
      'computed-property-spacing': 1,
      'func-call-spacing': 1,
      semi: [ 2, 'never' ],
      'space-before-blocks': [ 2, 'always' ],

      'space-before-function-paren': [ 2, {
        anonymous: 'always',
        named: 'never'
      } ],

      'space-in-parens': [ 'error', 'never' ],
      strict: 0,
      yoda: 1,
      'require-atomic-updates': 0,
      'quote-props': [ 'error', 'as-needed' ]
    }
  },
  ...compat.extends(
    'eslint:recommended',
    'plugin:@typescript-eslint/eslint-recommended',
    'plugin:@typescript-eslint/recommended'
  ).map((config) => ({
    ...config,
    files: [ 'test/*.ts' ]
  })),
  {
    files: [ 'test/*.ts' ],

    plugins: {
      '@typescript-eslint': typescriptEslint
    },

    languageOptions: {
      parser: tsParser
    },

    rules: {
      'no-extra-parens': 'off',
      '@typescript-eslint/no-unused-vars': [ 2, {
        ignoreRestSiblings: true,
        args: 'after-used',
        argsIgnorePattern: '^_',
        caughtErrorsIgnorePattern: '^_'
      } ]
    }
  },
  ...compat.extends(
    'eslint:recommended',
    'plugin:@typescript-eslint/eslint-recommended',
    'plugin:@typescript-eslint/recommended'
  ).map((config) => ({
    ...config,
    files: [ 'test/api_asyncIterators.test.js' ]
  })),
  {
    files: [ 'test/api_asyncIterators.test.js' ],

    plugins: {
      '@typescript-eslint': typescriptEslint
    },

    languageOptions: {
      parser: tsParser,
      ecmaVersion: 2018,
      sourceType: 'script'
    }
  },
  {
    files: [
      'test/stress.js',
      'lib/readable.js',
      'lib/writable.js',
      'lib/multiplexer.js',
      'bench/*.js',
      'examples/serve-async.js'
    ],

    languageOptions: {
      ecmaVersion: 2018,
      sourceType: 'script'
    }
  },
  {
    files: [ '**/*.mjs' ],

    languageOptions: {
      ecmaVersion: 2023,
      sourceType: 'module'
    }
  },
  {
    files: [ 'examples/electron/*' ],

    languageOptions: {
      globals: {
        ...globals.browser
      }
    }
  },
  {
    files: [ 'lib/gdal.js' ],

    languageOptions: {
      ecmaVersion: 2020
    }
  }
]
