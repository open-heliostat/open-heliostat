import Icons from 'unplugin-icons/vite';
import { defineConfig } from 'vitest/config';
import path from 'node:path';
import { svelte } from '@sveltejs/vite-plugin-svelte';

export default defineConfig({
	plugins: [
		svelte(),
		Icons({
			compiler: 'svelte'
		})
	],
	resolve: {
		conditions: ['browser'],
		alias: {
			'$lib/components/AccelCalibComp.svelte': path.resolve('src/test/stubs/AccelCalibComp.svelte'),
			'$lib': path.resolve('src/lib'),
			'$src': path.resolve('src')
		}
	},
	test: {
		environment: 'jsdom',
		setupFiles: ['src/test/setup.ts'],
		include: ['src/**/*.test.ts'],
		clearMocks: true,
		restoreMocks: true,
		mockReset: true
	}
});
