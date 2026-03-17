import { svelte } from '@sveltejs/vite-plugin-svelte';
import Icons from 'unplugin-icons/vite';
import path from 'node:path';
import { defineConfig } from 'vitest/config';

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
