import { fireEvent, render, screen, waitFor } from '@testing-library/svelte';
import { describe, expect, it, vi } from 'vitest';

import Heliostat from './Heliostat.svelte';

type MockState = {
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

function jsonResponse(data: unknown, status = 200): Response {
	return {
		status,
		json: async () => JSON.parse(JSON.stringify(data))
	} as Response;
}

function makeState(tiltDeg: number, tiltAzimuthDeg: number, azimuth = 180, elevation = 45): MockState {
	return {
		enabled: true,
		currentSource: 'Sun',
		currentTarget: 'None',
		sourcesMap: {
			Sun: { azimuth: 180, elevation: 45 }
		},
		mountOrientation: {
			tiltDeg,
			tiltAzimuthDeg
		},
		sunTracker: {
			latitude: 0,
			longitude: 0,
			isTimeSet: true,
			azimuth,
			elevation,
			utcIso: '2026-01-01T00:00:00Z',
			localIso: '2026-01-01T00:00:00+00:00',
			tz: 'UTC',
			offsetMinutes: 0
		}
	};
}

function getNumberInput(label: string): HTMLInputElement {
	const inputs = Array.from(document.querySelectorAll('input[type="number"]')) as HTMLInputElement[];
	const numberInput = inputs.find((input) => input.id === label);
	if (!numberInput) {
		throw new Error(`No numeric input found for label: ${label}`);
	}
	return numberInput;
}

function getRangeInput(label: string): HTMLInputElement {
	const inputs = Array.from(document.querySelectorAll('input[type="range"]')) as HTMLInputElement[];
	const rangeInput = inputs.find((input) => input.id === label);
	if (!rangeInput) {
		throw new Error(`No range input found for label: ${label}`);
	}
	return rangeInput;
}

describe('Heliostat orientation apply-readback loop', () => {
	it('posts mountOrientation payload and performs follow-up GET refresh', async () => {
		const initialState = makeState(0, 0, 180, 45);
		const refreshedState = makeState(15, 225, 210, 35);
		let latestState = initialState;

		const fetchMock = vi.fn(async (input: RequestInfo | URL, init?: RequestInit) => {
			const url = typeof input === 'string' ? input : input.toString();
			const method = (init?.method ?? 'GET').toUpperCase();
			if (url === '/rest/heliostat' && method === 'GET') {
				return jsonResponse(latestState);
			}
			if (url === '/rest/heliostat' && method === 'POST') {
				latestState = refreshedState;
				return jsonResponse(refreshedState);
			}
			return jsonResponse({}, 404);
		});

		Object.defineProperty(globalThis, 'fetch', {
			value: fetchMock,
			writable: true,
			configurable: true
		});

		render(Heliostat);
		await screen.findByText('Mount Orientation Setup');

		await fireEvent.change(getRangeInput('Tilt (deg)'), { target: { value: '15' } });
		await fireEvent.change(getRangeInput('Tilt Azimuth (deg)'), { target: { value: '225' } });
		await fireEvent.click(screen.getByRole('button', { name: /apply orientation/i }));

		const postCalls = fetchMock.mock.calls.filter(
			([url, init]) => String(url) === '/rest/heliostat' && (init?.method ?? 'GET').toUpperCase() === 'POST'
		);
		expect(postCalls.length).toBeGreaterThan(0);
		const postCall = postCalls.at(-1);
		const payload = JSON.parse(String(postCall?.[1]?.body));
		expect(Object.keys(payload)).toEqual(['mountOrientation']);
		expect(typeof payload.mountOrientation.tiltDeg).toBe('number');
		expect(typeof payload.mountOrientation.tiltAzimuthDeg).toBe('number');

		const getCalls = fetchMock.mock.calls.filter(
			([url, init]) => String(url) === '/rest/heliostat' && (init?.method ?? 'GET').toUpperCase() === 'GET'
		);
		expect(getCalls.length).toBeGreaterThanOrEqual(2);
	});

	it('shows apply success status and refreshed orientation values after apply', async () => {
		const initialState = makeState(0, 0, 180, 45);
		const refreshedState = makeState(15, 225, 210, 35);
		let latestState = initialState;

		Object.defineProperty(globalThis, 'fetch', {
			value: vi.fn(async (input: RequestInfo | URL, init?: RequestInit) => {
				const url = typeof input === 'string' ? input : input.toString();
				const method = (init?.method ?? 'GET').toUpperCase();
				if (url === '/rest/heliostat' && method === 'GET') {
					return jsonResponse(latestState);
				}
				if (url === '/rest/heliostat' && method === 'POST') {
					latestState = refreshedState;
					return jsonResponse(refreshedState);
				}
				return jsonResponse({}, 404);
			}),
			writable: true,
			configurable: true
		});

		render(Heliostat);
		await screen.findByText('Mount Orientation Setup');

		await fireEvent.change(getRangeInput('Tilt (deg)'), { target: { value: '15' } });
		await fireEvent.change(getRangeInput('Tilt Azimuth (deg)'), { target: { value: '225' } });
		await fireEvent.click(screen.getByRole('button', { name: /apply orientation/i }));

		await waitFor(() => {
			expect(screen.getByText(/orientation applied successfully/i)).toBeTruthy();
		});
		expect(getNumberInput('Tilt (deg)').value).toBe('15');
		expect(getNumberInput('Tilt Azimuth (deg)').value).toBe('225');
	});

	it('updates sun tracker observation values after read-back refresh', async () => {
		const initialState = makeState(0, 0, 180, 45);
		const refreshedState = makeState(15, 225, 210, 35);
		let latestState = initialState;

		Object.defineProperty(globalThis, 'fetch', {
			value: vi.fn(async (input: RequestInfo | URL, init?: RequestInit) => {
				const url = typeof input === 'string' ? input : input.toString();
				const method = (init?.method ?? 'GET').toUpperCase();
				if (url === '/rest/heliostat' && method === 'GET') {
					return jsonResponse(latestState);
				}
				if (url === '/rest/heliostat' && method === 'POST') {
					latestState = refreshedState;
					return jsonResponse(refreshedState);
				}
				return jsonResponse({}, 404);
			}),
			writable: true,
			configurable: true
		});

		render(Heliostat);
		await screen.findByText('Mount Orientation Setup');

		await fireEvent.change(getRangeInput('Tilt (deg)'), { target: { value: '15' } });
		await fireEvent.change(getRangeInput('Tilt Azimuth (deg)'), { target: { value: '225' } });
		await fireEvent.click(screen.getByRole('button', { name: /apply orientation/i }));

		await waitFor(() => {
			expect(screen.getByText(/azimuth : 210, elevation : 35/i)).toBeTruthy();
		});
	});

	it('hydrates orientation controls from /rest/heliostat on initial load', async () => {
		Object.defineProperty(globalThis, 'fetch', {
			value: vi.fn(async () => jsonResponse(makeState(12, 250, 200, 30))),
			writable: true,
			configurable: true
		});

		render(Heliostat);
		await screen.findByText('Mount Orientation Setup');

		await waitFor(() => {
			expect(getNumberInput('Tilt (deg)').value).toBe('12');
			expect(getNumberInput('Tilt Azimuth (deg)').value).toBe('250');
		});
	});
});
