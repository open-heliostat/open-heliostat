import { describe, expect, it } from 'vitest';
import { render, screen, waitFor } from '@testing-library/svelte';

import Heliostat from './Heliostat.svelte';

describe('Heliostat mount orientation setup', () => {
	it('renders a dedicated mount orientation setup section', async () => {
		render(Heliostat);

		await waitFor(() => {
			expect(screen.getByText('Mount Orientation Setup')).toBeTruthy();
		});
		expect(screen.getByText('Heliostat Control')).toBeTruthy();
		expect(screen.getByText('Sun Tracker')).toBeTruthy();
	});

	it('shows inline validation when tiltDeg is outside [-90, 90]', async () => {
		render(Heliostat);
		await screen.findByText('Mount Orientation Setup');

		expect(screen.getByText(/tilt must be between -90 and 90 degrees/i)).toBeTruthy();
	});

	it('shows inline validation when tiltAzimuthDeg is outside [0, 360]', async () => {
		render(Heliostat);
		await screen.findByText('Mount Orientation Setup');

		expect(screen.getByText(/tilt azimuth must be between 0 and 360 degrees/i)).toBeTruthy();
	});

	it('disables apply while validation errors are present', async () => {
		render(Heliostat);
		await screen.findByText('Mount Orientation Setup');

		const applyButton = screen.getByRole('button', { name: /apply orientation/i });
		expect(applyButton.hasAttribute('disabled')).toBe(true);
	});
});
