import type { PageLoad } from './$types';

export const load = (async ({ fetch }) => {
	return {
		title: 'Steppers'
	};
}) satisfies PageLoad;
