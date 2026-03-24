import { describe, expect, it } from 'vitest';
import { fireEvent, render, screen, waitFor } from '@testing-library/svelte';

import Heliostat from './Heliostat.svelte';

describe('Heliostat orientation resolve flow', () => {
	it('starts the guided resolve flow and refreshes its status', async () => {
		render(Heliostat);
		await screen.findByText('Mount Orientation Setup');

		await fireEvent.click(screen.getByRole('button', { name: /auto-resolve orientation/i }));

		await waitFor(() => {
			expect(screen.getByText('moving')).toBeTruthy();
		});
		expect(screen.getByText(/0\/12 poses, RMS 0.000/i)).toBeTruthy();
	});
});