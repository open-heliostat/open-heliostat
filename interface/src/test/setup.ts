import { cleanup } from '@testing-library/svelte';
import { afterEach, beforeEach, vi } from 'vitest';

type MockHeliostatState = {
	enabled: boolean;
	currentSource: string;
	currentTarget: string;
	sourcesMap: Record<string, { azimuth: number; elevation: number }>;
	mountOrientation: { tiltDeg: number; tiltAzimuthDeg: number };
	sunTracker: {
		latitude: number;
		longitude: number;
		isTimeSet: boolean;
		azimuth: number;
		elevation: number;
		utcIso?: string;
		localIso?: string;
		tz?: string;
		offsetMinutes?: number;
	};
};

const baseState: MockHeliostatState = {
	enabled: true,
	currentSource: 'Sun',
	currentTarget: 'None',
	sourcesMap: {
		Sun: { azimuth: 180, elevation: 45 }
	},
	mountOrientation: {
		tiltDeg: 0,
		tiltAzimuthDeg: 0
	},
	sunTracker: {
		latitude: 0,
		longitude: 0,
		isTimeSet: true,
		azimuth: 180,
		elevation: 45,
		utcIso: '2026-01-01T00:00:00Z',
		localIso: '2026-01-01T00:00:00+00:00',
		tz: 'UTC',
		offsetMinutes: 0
	}
};

let mockState: MockHeliostatState = JSON.parse(JSON.stringify(baseState));

function clone<T>(value: T): T {
	return JSON.parse(JSON.stringify(value));
}

function response(data: unknown, status = 200): Response {
	return {
		status,
		json: async () => clone(data)
	} as Response;
}

function mergePatch(body: unknown): void {
	if (!body || typeof body !== 'object') {
		return;
	}

	const patch = body as Record<string, unknown>;
	if (patch.mountOrientation && typeof patch.mountOrientation === 'object') {
		const orientation = patch.mountOrientation as Record<string, unknown>;
		if (typeof orientation.tiltDeg === 'number') {
			mockState.mountOrientation.tiltDeg = orientation.tiltDeg;
		}
		if (typeof orientation.tiltAzimuthDeg === 'number') {
			mockState.mountOrientation.tiltAzimuthDeg = orientation.tiltAzimuthDeg;
		}
	}
}

function setupFetchMock(): void {
	const fetchMock = vi.fn(async (input: RequestInfo | URL, init?: RequestInit) => {
		const url = typeof input === 'string' ? input : input.toString();
		const method = (init?.method ?? 'GET').toUpperCase();

		if (url === '/rest/heliostat' && method === 'GET') {
			return response(mockState);
		}

		if (url === '/rest/heliostat' && method === 'POST') {
			const body = init?.body ? JSON.parse(String(init.body)) : {};
			mergePatch(body);
			return response(mockState);
		}

		if (url.endsWith('/sunTracker') && method === 'POST') {
			return response(mockState);
		}

		return response({}, 404);
	});

	Object.defineProperty(globalThis, 'fetch', {
		value: fetchMock,
		writable: true,
		configurable: true
	});
}

function applyMockStatePatch(patch: Partial<MockHeliostatState>): void {
	mockState = {
		...mockState,
		...patch,
		mountOrientation: {
			...mockState.mountOrientation,
			...(patch.mountOrientation ?? {})
		},
		sunTracker: {
			...mockState.sunTracker,
			...(patch.sunTracker ?? {})
		}
	};
}

beforeEach(() => {
	mockState = clone(baseState);
	setupFetchMock();
	Object.defineProperty(globalThis, '__setMockHeliostatState', {
		value: applyMockStatePatch,
		writable: true,
		configurable: true
	});
});

afterEach(() => {
	cleanup();
	vi.restoreAllMocks();
});
